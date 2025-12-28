package config

import (
	"DirkBuildTool/models"
	"encoding/json"
	"fmt"
	"log"
	"os"
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

var BuildTypes map[string]*models.BuildType
var Platform PlatformConfig
var Dirs DirsConfig
var Settings BuildToolSettings

const settingsFile = "settings.json"

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

	BuildTypes = map[string]*models.BuildType{
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
