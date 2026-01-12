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

type BuildConfig struct {
	target config.Target
	mode   config.BuildMode

	graph   Graph
	defines config.Defines
}

func (c *BuildConfig) getModules() map[string]module.Module {
	return c.graph.getNodes()
}

func (c *BuildConfig) GenerateCompileCommands() error {
	log.Printf("Generating compile commands\n")
	compileCommands := config.CompileCommands{}
	for _, buildMod := range c.getModules() {
		mod, ok := buildMod.(*module.CppModule)
		if !ok {
			continue
		}

		commands, err := mod.GenerateCompileCommands(c.defines)
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
	return nil
}

func (c *BuildConfig) Build() error {
	log.Printf("Building target %s with %s configuration\n", c.target.Name, c.mode.Name)
	buildModules, err := c.graph.flatten()
	if err != nil {
		return err
	}

	for _, mod := range buildModules {
		if err := mod.Build(&c.target, c.defines); err != nil {
			if _, ok := err.(*exec.ExitError); ok {
				fmt.Printf("An error occured in the build process\n")
				return nil
			}
			return err
		}
	}

	return nil
}

func SetupBuildConfig(target config.Target, mode config.BuildMode) (*BuildConfig, error) {
	modules, err := searchDir(config.Dirs.Source, &mode, 0)
	if err != nil {
		return nil, err
	}

	modules["Shaders"], err = module.Load(config.Dirs.Engine, "Shaders", &mode)
	if err != nil {
		return nil, err
	}

	graph, err := buildDependencyGraph(modules)
	if err != nil {
		return nil, err
	}

	targets := []module.Module{}
	for _, targetName := range target.Modules {
		targetMod, ok := modules[targetName]
		if !ok {
			return nil, fmt.Errorf("module %s required by target %s does not exist", targetName, target.Name)
		}

		targets = append(targets, targetMod)
	}

	graph, err = graph.strip(targets)
	if err != nil {
		return nil, err
	}

	defines := target.Defines
	if defines == nil {
		defines = config.Defines{}
	}

	for _, mod := range graph.getNodes() {
		if mod.GetDefines() != nil {
			maps.Copy(defines, mod.GetDefines())
		}
	}

	if config.Platform.Defines != nil {
		maps.Copy(defines, config.Platform.Defines)
	}

	if mode.Defines != nil {
		maps.Copy(defines, mode.Defines)
	}

	defines["SAVED_DIR"] = fmt.Sprintf("%s/%s/%s", config.Dirs.Saved, target.Name, mode.Name)
	defines["SHADERS_DIR"] = fmt.Sprintf("%s/Shaders", config.Dirs.Intermediate)
	defines["ASSETS_DIR"] = config.Dirs.Assets

	return &BuildConfig{target, mode, graph, defines}, nil
}

func searchDir(path string, buildMode *config.BuildMode, count int) (map[string]module.Module, error) {
	entries, err := os.ReadDir(path)
	if err != nil {
		return nil, err
	}

	if count >= 10 {
		log.Printf("Module search has reached %d recursions. Currently searching: %s.\n", count, path)
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

		config, err := module.Load(path, info.Name(), buildMode)
		if err != nil {
			return nil, err
		}

		if config == nil {
			newMods, err := searchDir(fmt.Sprintf("%s/%s", path, entry.Name()), buildMode, count+1)
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
