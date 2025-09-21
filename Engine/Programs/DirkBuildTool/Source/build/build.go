package build

import (
	"DirkBuildTool/module"
	"DirkBuildTool/output"
	"DirkBuildTool/setup"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"os"
	"os/exec"
)

func Build(buildConfig *setup.BuildConfig) error {
	log.Printf("Building %s for %s\n", buildConfig.Target, buildConfig.Type.Name)
	configs, err := searchDir(output.Dirs.Source)
	if err != nil {
		return err
	}

	modules := map[string]module.Module{}
	for name, config := range configs {
		modules[name] = config.ToModule(buildConfig)
	}

	target, ok := modules[buildConfig.Target]
	if !ok {
		log.Printf("Target %s does not exist, skipping", buildConfig.Target)
		return nil
	}

	log.Printf("Resolving dependencies")
	target.ResolveDependencies(modules, nil)
	err = target.Build()
	if err == nil {
		return nil
	}
	if errors.Is(err, &exec.ExitError{}) {
		fmt.Printf("An error occured in build process. See previous errors for details")
		return nil
	}
	return err
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
				log.Printf("Error loading module %s: %s\n", name, err.Error())
				return nil, nil
			}

			if config.Target == "" {
				config.Target = config.Name
			}

			return config, nil
		}
	}

	return nil, nil
}
