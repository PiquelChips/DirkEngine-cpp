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

	target, ok := modules[buildConfig.Target.Module]
	if !ok {
		log.Printf("Module %s specified by target %s does not exist, skipping\n", buildConfig.Target.Module, buildConfig.Target.Name)
		return nil
	}

	if cppTarget, ok := target.(*module.CppModule); ok {
		log.Printf("Resolving dependencies\n")
		if err := cppTarget.ResolveDependencies(modules, nil); err != nil {
			return err
		}

		log.Printf("Generating compile commands\n")
		compileCommands, err := cppTarget.GenerateCompileCommands()
		if err != nil {
			return err
		}

		data, err := json.Marshal(compileCommands)
		if err != nil {
			return err
		}

		if err := config.SaveFile("compile_commands.json", data, true); err != nil {
			return err
		}

		os.Symlink(fmt.Sprintf("%s/compile_commands.json", config.Dirs.DBTSaved), fmt.Sprintf("%s/compile_commands.json", config.Dirs.Work))
	}

	if err := module.Build(target); err == nil {
		return nil
	} else if _, ok := err.(*exec.ExitError); ok {
		fmt.Printf("An error occured in the build process\n")
		return nil
	} else {
		return err
	}
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
