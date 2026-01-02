package config

import (
	"DirkBuildTool/models"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"slices"
	"strings"
)

const DirPerm = 0755
const FilePerm = 0644

type DirsConfig struct {
	Work, Engine, Config,
	Source, Thirdparty,
	Intermediate, Binaries, Saved,
	DBTSaved, DBTConfig string
}

type BuildToolSettings struct {
	LibSearchEnvs []string `json:"lib_search_envs"`
}

type PlatformConfig struct {
	Name    string
	Defines map[string]string
}

type ThirdpartyConfig map[string]*models.ThirdpartyDependency

var BuildModes map[string]*models.BuildMode
var Platform PlatformConfig
var Dirs DirsConfig
var Settings BuildToolSettings
var Thirdparty ThirdpartyConfig // TODO: actually load this

const settingsFile = "settings.json"
const thirdpartyFile = "thirdparty.json"

func LoadConfig() error {
	log.Printf("Loading configuration\n")

	Platform = detectPlatform()

	wd, err := os.Getwd()
	if err != nil {
		return err
	}

	Dirs = setupDirsConfig(wd)

	if err := os.MkdirAll(Dirs.Intermediate, DirPerm); err != nil {
		return err
	}
	if err := os.MkdirAll(Dirs.Binaries, DirPerm); err != nil {
		return err
	}
	if err := os.MkdirAll(Dirs.DBTSaved, DirPerm); err != nil {
		return err
	}

	BuildModes = map[string]*models.BuildMode{
		"Development": {
			Name:     "Development",
			Optimize: false,
			Compact:  false,
			Defines: map[string]string{
				"DIRK_DEVELOPMENT_BUILD": "",
				"DIRK_DEBUG_BUILD":       "",
			},
		},
		"Shipping": {
			Name:     "Shipping",
			Optimize: true,
			Compact:  true,
			Defines: map[string]string{
				"DIRK_SHIPPING_BUILD": "",
			},
		},
	}

	Settings, err = loadSettings()
	if err != nil {
		return err
	}

	Thirdparty, err = loadThirdparty()
	if err != nil {
		return err
	}

	// TODO: load targets

	return nil
}

func detectPlatform() PlatformConfig {
	return PlatformConfig{
		Name:    "Linux",
		Defines: map[string]string{"PLATFORM_LINUX": ""},
	}
}

func setupDirsConfig(wd string) DirsConfig {
	engineDir := fmt.Sprintf("%s/Engine", wd)
	savedDir := fmt.Sprintf("%s/Saved", engineDir)
	configDir := fmt.Sprintf("%s/Config", engineDir)
	return DirsConfig{
		Work:         wd,
		Engine:       engineDir,
		Config:       configDir,
		Source:       fmt.Sprintf("%s/Source", engineDir),
		Thirdparty:   fmt.Sprintf("%s/Thirdparty", engineDir),
		Intermediate: fmt.Sprintf("%s/Intermediate", engineDir),
		Binaries:     fmt.Sprintf("%s/Binaries", engineDir),
		Saved:        savedDir,
		DBTSaved:     fmt.Sprintf("%s/DirkBuildTool", savedDir),
		DBTConfig:    fmt.Sprintf("%s/DirkBuildTool", configDir),
	}
}

func loadSettings() (BuildToolSettings, error) {
	config := BuildToolSettings{}
	if err := LoadConfigFile(settingsFile, &config); err != nil {
		return BuildToolSettings{}, err
	}
	return config, nil
}

func loadThirdparty() (ThirdpartyConfig, error) {
	thirdparty := ThirdpartyConfig{}
	if err := LoadConfigFile(thirdpartyFile, &thirdparty); err != nil {
		return ThirdpartyConfig{}, err
	}

	getDir := func(name string) (string, error) {
		return filepath.Abs(fmt.Sprintf("%s/%s", Dirs.Thirdparty, name))
	}

	getLibraryPaths := func(envVars []string) []string {
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

	// fixup some stuff
	externalLibs := []string{}
	for name, dep := range thirdparty {
		if dep.IncludeDir == "" {
			dep.IncludeDir = "include"
		}

		if len(dep.Libs) == 0 && (dep.External || !dep.IsHeaderOnly) {
			dep.Libs = []string{name}
		}

		if !filepath.IsAbs(dep.IncludeDir) {
			dir, err := getDir(name)
			if err != nil {
				return ThirdpartyConfig{}, err
			}

			incDir, err := filepath.Abs(fmt.Sprintf("%s/%s", dir, dep.IncludeDir))
			if err != nil {
				return ThirdpartyConfig{}, err
			}
			dep.IncludeDir = incDir
		}

		// for linking external libs
		if dep.External {
			externalLibs = append(externalLibs, dep.GetLibs()...)
		}
	}

	paths := getLibraryPaths(Settings.LibSearchEnvs)
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
					os.Symlink(fmt.Sprintf("%s/%s", path, entry.Name()), fmt.Sprintf("%s/%s", Dirs.Binaries, entry.Name()))
				}
			}
		}
	}

	return thirdparty, nil
}

func LoadConfigFile(file string, out any) error {
	data, err := os.ReadFile(fmt.Sprintf("%s/%s", Dirs.DBTConfig, file))
	if err != nil {
		return fmt.Errorf("Error loading config file: %w", err)
	}

	if err := json.Unmarshal(data, out); err != nil {
		return fmt.Errorf("Error parsing config file: %w", err)
	}

	return nil
}

func SaveFile(name string, data []byte, overwrite bool) error {
	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", Dirs.DBTSaved, name)

	if overwrite {
		return os.WriteFile(name, data, FilePerm)
	}

	f, err := os.OpenFile(name, os.O_APPEND|os.O_CREATE, FilePerm)
	if err != nil {
		return err
	}
	defer f.Close()

	f.Write(data)
	return nil
}

func ReadSavedFile(name string) ([]byte, error) {
	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", Dirs.DBTSaved, name)
	return os.ReadFile(name)
}

func GetSavedFile(name string) (os.FileInfo, error) {
	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", Dirs.DBTSaved, name)

	return os.Stat(name)
}
