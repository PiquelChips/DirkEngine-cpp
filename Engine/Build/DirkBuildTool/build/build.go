package build

import (
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"DirkBuildTool/setup"
	"encoding/json"
	"fmt"
	"os"
)

func Build() error {
	fmt.Printf("build has not been implemented yet. this is a work in progress\n")
	thirdparty, err := setup.ReadThirdparty()
	if err != nil {
		return err
	}

	configs, err := searchDir(output.Dirs.Source)
	if err != nil {
		return err
	}

	final := configs["Editor"]

	targetConfigs := map[string]*models.ModuleConfig{final.Name: final}
	addDeps(targetConfigs, configs, final)

	targets := map[string]*models.Module{}
	for name, config := range targetConfigs {
		deps := []*models.Dependency{}
		for _, depName := range config.Deps {
			deps = append(deps, targetConfigs[depName].ToDependency())
		}
		for _, depName := range config.Ext {
			deps = append(deps, thirdparty[depName])
		}

		targets[name] = &models.Module{
			Name:    config.Name,
			Path:    config.Path,
			Std:     config.Std,
			IsLib:   config.IsLib,
			Deps:    deps,
			Defines: config.Defines,
		}
	}

	// open compile commands file
	// loop over every target but final & build
	// build final
	// close compile commands

	/**
	 * BUILD OPTIONS:
	 * - shipping -- static linking & all optimizations
	 * - dev -- shared linking & no optimizations
	 */

	return nil
}

func searchDir(path string) (map[string]*models.ModuleConfig, error) {
	entries, err := os.ReadDir(output.Dirs.Source)
	if err != nil {
		return nil, err
	}

	configs := map[string]*models.ModuleConfig{}
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

func getMod(path, name string) (*models.ModuleConfig, error) {
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

			config := &models.ModuleConfig{}
			err = json.Unmarshal(data, config)
			if err != nil {
				fmt.Printf("Error in %s.dirkmod: %s\n", name, err.Error())
				return nil, nil
			}

			config.Path = path
			return config, nil
		}
	}

	return nil, nil
}

func addDeps(targetConfigs map[string]*models.ModuleConfig, configs map[string]*models.ModuleConfig, dep *models.ModuleConfig) {
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
