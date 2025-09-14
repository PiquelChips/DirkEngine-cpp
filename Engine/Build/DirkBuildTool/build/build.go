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
	_, err := setup.ReadThirdparty()
	if err != nil {
		return err
	}

	_, err = searchDir(output.Dirs.Source)
	if err != nil {
		return err
	}

	//var targets map[string]*models.Module

	/**
	 * BUILD OPTIONS:
	 * - shipping -- static linking & all optimizations
	 * - dev -- shared linking & no optimizations
	 */

	/**
	 * BUILD FLOW
	 * + load saved thirdparty dependencies
	 * - look for target in src dir (for now will be Editor)
	 * - resolve dependencies (ignore duplicates)
	 * - open compile commands
	 * - build every target
	 *   - main target should be last (Editor)
	 *   - generate makefile & run
	 * - close compile commands
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
	entries, err := os.ReadDir(fmt.Sprintf("%s/%s", path, name))
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
			data, err := os.ReadFile(fmt.Sprintf("%s/%s/%s.dirkmod", path, name, name))
			if err != nil {
				return nil, err
			}

			config := &models.ModuleConfig{}
			err = json.Unmarshal(data, config)
			if err != nil {
				fmt.Printf("Error in %s.dirkmod: %s", name, err.Error())
				return nil, nil
			}

			return config, nil
		}
	}

	return nil, nil
}
