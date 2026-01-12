package build

import (
	"DirkBuildTool/config"
	"DirkBuildTool/module"
	"encoding/json"
	"fmt"
	"log"
	"maps"
	"os"
	"os/exec"
)

func Build(buildConfig *config.BuildConfig) error {
	modules, err := searchDir(config.Dirs.Source, buildConfig, 0)
	if err != nil {
		return err
	}

	modules["Shaders"], err = module.Load(config.Dirs.Engine, "Shaders", buildConfig)
	if err != nil {
		return err
	}

	graph, err := buildDependencyGraph(modules)
	if err != nil {
		return err
	}

	targets := []module.Module{}
	for _, targetName := range buildConfig.Target.Modules {
		target, ok := modules[targetName]
		if !ok {
			return fmt.Errorf("module %s required by target %s does not exist", targetName, buildConfig.Target.Name)
		}

		targets = append(targets, target)
	}

	graph, err = graph.strip(targets)
	if err != nil {
		return err
	}

	buildModules, err := graph.flatten()
	if err != nil {
		return err
	}

	defines := buildConfig.Target.Defines
	if defines == nil {
		defines = config.Defines{}
	}

	for _, mod := range buildModules {
		if mod.GetDefines() != nil {
			maps.Copy(defines, mod.GetDefines())
		}
	}

	if config.Platform.Defines != nil {
		maps.Copy(defines, config.Platform.Defines)
	}

	if buildConfig.Mode.Defines != nil {
		maps.Copy(defines, buildConfig.Mode.Defines)
	}

	defines["SAVED_DIR"] = fmt.Sprintf("%s/%s/%s", config.Dirs.Saved, buildConfig.Target.Name, buildConfig.Mode.Name)
	defines["SHADERS_DIR"] = fmt.Sprintf("%s/Shaders", config.Dirs.Intermediate)
	defines["ASSETS_DIR"] = config.Dirs.Assets

	log.Printf("Generating compile commands\n")
	compileCommands := config.CompileCommands{}
	for _, buildMod := range buildModules {
		mod, ok := buildMod.(*module.CppModule)
		if !ok {
			continue
		}

		commands, err := mod.GenerateCompileCommands(defines)
		if err != nil {
			return err
		}

		compileCommands = append(compileCommands, commands...)
	}

	data, err := json.Marshal(compileCommands)
	if err != nil {
		return err
	}
	if err := config.SaveFile("compile_commands.json", data, true); err != nil {
		return err
	}
	os.Symlink(fmt.Sprintf("%s/compile_commands.json", config.Dirs.DBTSaved), fmt.Sprintf("%s/compile_commands.json", config.Dirs.Work))

	log.Printf("Building target %s with %s configuration\n", buildConfig.Target.Name, buildConfig.Mode.Name)
	for _, mod := range buildModules {
		if err := mod.Build(defines); err != nil {
			if _, ok := err.(*exec.ExitError); ok {
				fmt.Printf("An error occured in the build process\n")
				return nil
			}
			return err
		}
	}

	return nil
}

func searchDir(path string, buildConfig *config.BuildConfig, count int) (map[string]module.Module, error) {
	entries, err := os.ReadDir(path)
	if err != nil {
		return nil, err
	}

	if count >= 10 {
		log.Printf("Module search has reached %d recursions. Current dir: %s.\n", count, path)
	}

	modules := map[string]module.Module{}
	for _, entry := range entries {
		if !entry.IsDir() {
			continue
		}

		info, err := entry.Info()
		if err != nil {
			return nil, err
		}

		config, err := module.Load(path, info.Name(), buildConfig)
		if err != nil {
			return nil, err
		}

		if config == nil {
			newMods, err := searchDir(fmt.Sprintf("%s/%s", path, entry.Name()), buildConfig, count+1)
			if err != nil {
				return nil, err
			}
			if newMods != nil {
				maps.Copy(modules, newMods)
			}
		} else {
			modules[info.Name()] = config
		}
	}

	return modules, nil
}
