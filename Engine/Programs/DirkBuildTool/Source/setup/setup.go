package setup

import (
	"DirkBuildTool/build"
	"DirkBuildTool/config"
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"slices"
	"strings"
	"time"
)

const setupFile = "setup.json"
const thirdpartyFile = "thirdparty.json"

func isSetupValid(buildConfig *models.BuildConfig) bool {
	// attempt to read setup file
	data, err := output.ReadIntFile(setupFile)
	if err != nil {
		return false
	}

	// check json is valid
	if err := json.Unmarshal(data, config.Setup); err != nil {
		return false
	}

	// get file info
	info, err := output.GetIntFileInfo(setupFile)
	if err != nil {
		return false
	}

	// check dif between LastSetup & last file update
	if config.Setup.LastSetup.Sub(info.ModTime()).Seconds() > 1 {
		return false
	}

	if config.Setup.BuildConfig != buildConfig {
		return false
	}

	return true
}

func Setup(buildConfig *models.BuildConfig) error {
	config.Setup = &models.SetupConfig{}
	if isSetupValid(buildConfig) {
		return nil
	}
	log.Printf("No valid setup file detected. Running setup\n")

	config.Setup.LastSetup = time.Now()
	config.Setup.BuildConfig = buildConfig
	config.Setup.Platform = "Linux"

	os.Symlink(fmt.Sprintf("%s/compile_commands.json", config.Dirs.Intermediate), fmt.Sprintf("%s/compile_commands.json", config.Dirs.Root))

	data, err := os.ReadFile(fmt.Sprintf("%s/%s", config.Dirs.Thirdparty, thirdpartyFile))
	if err != nil {
		return err
	}

	if err := json.Unmarshal(data, &config.Setup.Thirdparty); err != nil {
		return err
	}

	// fixup some stuff
	externalLibs := []string{}
	for name, dep := range config.Setup.Thirdparty {
		if dep.IncludeDir == "" {
			dep.IncludeDir = "include"
		}

		if len(dep.Libs) == 0 && (dep.External || !dep.IsHeaderOnly) {
			dep.Libs = []string{name}
		}

		if !filepath.IsAbs(dep.IncludeDir) {
			dir, err := getDir(name)
			if err != nil {
				return err
			}

			incDir, err := filepath.Abs(fmt.Sprintf("%s/%s", dir, dep.IncludeDir))
			if err != nil {
				return err
			}
			dep.IncludeDir = incDir
		}

		// for linking external libs
		if dep.External {
			externalLibs = append(externalLibs, dep.GetLibs()...)
		}
	}

	paths := getLibraryPaths(config.General.LibSearchEnvs)
	for _, lib := range externalLibs {
		for _, path := range paths {
			entries, err := os.ReadDir(path)
			if err != nil {
				continue
			}
			for _, entry := range entries {
				if entry.IsDir() {
					continue
				}

				if strings.HasPrefix(entry.Name(), fmt.Sprintf("lib%s.so", lib)) {
					os.Symlink(fmt.Sprintf("%s/%s", path, entry.Name()), fmt.Sprintf("%s/%s", config.Dirs.Binaries, entry.Name()))
				}
			}
		}
	}

	// build everything thats left
	for name, dep := range config.Setup.Thirdparty {
		if dep.External || dep.IsHeaderOnly {
			continue
		}

		// TODO: fix compile commands generation
		if err := build.Build(&models.BuildConfig{
			Target: name,
			Type: &models.BuildType{
				Name:         "Thirdparty",
				Optimize:     true,
				Compact:      buildConfig.Type.Compact,
				Defines:      nil,
				WarningLevel: 0, // no warnings for thirdparty stuff
			},
			SearchDirs:     []string{config.Dirs.Thirdparty},
			ErrOnBuildFail: true,
		}); err != nil {
			return err
		}
	}

	// write the file
	data, err = json.Marshal(config.Setup)
	if err != nil {
		return nil
	}

	log.Printf("Writting setup file\n")
	return output.WriteIntFile(setupFile, data, true)
}

func getDir(name string) (string, error) {
	return filepath.Abs(fmt.Sprintf("%s/%s", config.Dirs.Thirdparty, name))
}

func getLibraryPaths(envVars []string) []string {
	paths := []string{}

	for _, envVar := range envVars {
		env := os.Getenv(envVar)
		for path := range strings.SplitSeq(env, ":") {
			if path == "" {
				continue
			}
			if slices.Contains(paths, path) {
				continue
			}

			paths = append(paths, path)
		}
	}

	return paths
}
