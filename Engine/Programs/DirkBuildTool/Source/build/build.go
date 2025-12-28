package build

import (
	"DirkBuildTool/models"
	"DirkBuildTool/module"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"os/exec"
)

func Build(buildConfig *models.BuildConfig) error {
	modules := map[string]module.Module{}
	for _, dir := range buildConfig.SearchDirs {
		modConfigs, err := searchDir(dir)
		if err != nil {
			return err
		}

		for name, conf := range modConfigs {
			if _, ok := modules[name]; ok {
				log.Printf("Module %s is declared twice\n", name)
			} else {
				modules[name] = conf.ToModule(buildConfig)
			}
		}
	}

	target, ok := modules[buildConfig.Target]
	if !ok {
		log.Printf("Target %s does not exist, skipping\n", buildConfig.Target)
		return nil
	}

	if cppTarget, ok := target.(*module.CppModule); ok {
		log.Printf("Resolving dependencies\n")
		if err := cppTarget.ResolveDependencies(modules, nil); err != nil {
			return err
		}

		compileCommands, err := cppTarget.GenerateCompileCommands()
		if err != nil {
			return err
		}

		data, err := json.Marshal(compileCommands)
		if err != nil {
			return err
		}

		if err := output.WriteIntFile("compile_commands.json", data, true); err != nil {
			return err
		}
	}

	if err := module.Build(target); err == nil {
		return nil
	} else if _, ok := err.(*exec.ExitError); ok {
		fmt.Printf("An error occured in the build process\n")
		if buildConfig.ErrOnBuildFail {
			return err
		}
		return nil
	} else {
		return err
	}
}

func searchDir(path string) (map[string]*module.ModuleConfig, error) {
	entries, err := os.ReadDir(path)
	if err != nil {
		return nil, err
	}

	configs := map[string]*module.ModuleConfig{}
	for _, entry := range entries {
		if !entry.IsDir() {
			continue
		}

		info, err := entry.Info()
		if err != nil {
			return nil, err
		}

		config, err := getMod(path, info.Name())
		if err != nil {
			return nil, err
		}
		if config != nil {
			configs[info.Name()] = config
		}
	}

	return configs, nil
}

func getMod(path, name string) (*module.ModuleConfig, error) {
	path = fmt.Sprintf("%s/%s", path, name)
	modFile := fmt.Sprintf("%s/%s.dirkmod", path, name)
	data, err := os.ReadFile(modFile)
	if err != nil {
		return nil, nil
	}

	config := &module.ModuleConfig{}
	err = json.Unmarshal(data, config)
	if err != nil {
		log.Printf("Error loading module %s: %s\n", name, err.Error())
		return nil, nil
	}

	if config.Target == "" {
		config.Target = config.Name
	}

	config.Path = path

	return config, nil
}
