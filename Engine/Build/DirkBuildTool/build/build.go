package build

import (
	"DirkBuildTool/module"
	"DirkBuildTool/output"
	"DirkBuildTool/setup"
	"encoding/json"
	"fmt"
	"os"
)

func Build(targetName string) error {
	thirdparty, err := setup.ReadThirdparty()
	if err != nil {
		return err
	}

	configs, err := searchDir(output.Dirs.Source)
	if err != nil {
		return err
	}

	modules := map[string]*module.Module{}
	for name, config := range configs {
		modules[name] = config.ToModule()
	}

	target, ok := modules[targetName]
	if !ok {
		fmt.Printf("target %s does not exist", targetName)
		return nil
	}
	module.ResolveDependencies(target, modules, thirdparty)

	/*
		compileCommandsPath := fmt.Sprintf("%s/compile_commands.json", output.Dirs.Root)
		if err := os.WriteFile(compileCommandsPath, []byte("["), output.FilePerm); err != nil {
			return nil
		}
	*/

	target.Build()

	/*
		f, err := os.OpenFile(compileCommandsPath, os.O_APPEND|os.O_CREATE, output.FilePerm)
		if err != nil {
			return err
		}
		defer f.Close()

		if _, err := f.Write([]byte("{}]")); err != nil {
			return err
		}
	*/

	/**
	 * BUILD OPTIONS:
	 * - shipping -- static linking & all optimizations
	 * - dev -- shared linking & no optimizations
	 */

	return nil
}

func searchDir(path string) (map[string]*module.ModuleConfig, error) {
	entries, err := os.ReadDir(output.Dirs.Source)
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
	entries, err := os.ReadDir(path)
	if err != nil {
		return nil, err
	}

	for _, entry := range entries {
		if entry.IsDir() {
			continue
		}

		info, err := entry.Info()
		if err != nil {
			return nil, err
		}

		if info.Name() == fmt.Sprintf("%s.dirkmod", name) {
			data, err := os.ReadFile(fmt.Sprintf("%s/%s.dirkmod", path, name))
			if err != nil {
				return nil, err
			}

			config := &module.ModuleConfig{}
			err = json.Unmarshal(data, config)
			if err != nil {
				fmt.Printf("Error in %s.dirkmod: %s\n", name, err.Error())
				return nil, nil
			}

			return config, nil
		}
	}

	return nil, nil
}

func addDeps(targetConfigs map[string]*module.ModuleConfig, configs map[string]*module.ModuleConfig, dep *module.ModuleConfig) {
	startLen := len(targetConfigs)
	for _, depName := range dep.Deps {
		dep, ok := configs[depName]
		if !ok {
			fmt.Printf("dependency %s referenced in module %s does not exist\n", depName, dep.Name)
			continue
		}

		targetConfigs[depName] = dep
	}

	if len(targetConfigs) != startLen {
		for _, conf := range targetConfigs {
			addDeps(targetConfigs, configs, conf)
		}
	}
}
