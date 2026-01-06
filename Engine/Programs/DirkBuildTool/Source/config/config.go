package config

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"strings"
)

const (
	DirPerm  = 0755
	FilePerm = 0644
)

const (
	WarningLevelNone   = 0
	WarningLevelLow    = 1
	WarningLevelMedium = 2
	WarningLevelMax    = 3
)

type BuildConfig struct {
	Target Target
	Mode   *BuildMode
}

type Defines map[string]string

type CompileCommands []*CompileCommand

type CompileCommand struct {
	Directory string   `json:"directory"`
	Arguments []string `json:"arguments"`
	File      string   `json:"file"`
	Output    string   `json:"output"`
}

type BuildMode struct {
	Name         string
	Optimize     bool
	Compact      bool // compact the output (essentially statically linking)
	Defines      map[string]string
	LinkerFlags  []string
	CompileFlags []string
	WarningLevel int
}

type DirsConfig struct {
	Work, Engine, Config,
	Source, Assets,
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

type Target struct {
	Name    string   `json:"name"`
	Defines Defines  `json:"defines"`
	Modules []string `json:"modules"`
}

var (
	BuildModes map[string]*BuildMode
	Platform   PlatformConfig
	Dirs       DirsConfig
	Settings   BuildToolSettings
	Targets    map[string]Target
)

const (
	settingsFile = "settings.json"
)

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

	BuildModes = map[string]*BuildMode{
		"Development": {
			Name:     "Development",
			Optimize: false,
			Compact:  false,
			Defines: map[string]string{
				"DIRK_DEVELOPMENT_BUILD": "",
				"DIRK_DEBUG_BUILD":       "",
			},
			LinkerFlags:  []string{"-g"},
			CompileFlags: []string{"-g"},
			WarningLevel: WarningLevelMedium,
		},
		"Shipping": {
			Name:     "Shipping",
			Optimize: true,
			Compact:  true,
			Defines: map[string]string{
				"DIRK_SHIPPING_BUILD": "",
			},
			LinkerFlags:  []string{},
			CompileFlags: []string{},
			WarningLevel: WarningLevelMax,
		},
	}

	Settings, err = loadSettings()
	if err != nil {
		return err
	}

	Targets, err = loadTargets()
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
		Assets:       fmt.Sprintf("%s/Assets", engineDir),
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

func loadTargets() (map[string]Target, error) {
	entries, err := os.ReadDir(Dirs.Source)
	if err != nil {
		return nil, err
	}

	targets := map[string]Target{}
	for _, entry := range entries {
		if !strings.HasSuffix(entry.Name(), ".dirktarget") {
			continue
		}
		targetName := strings.TrimSuffix("DirkEditor.dirktarget", ".dirktarget")

		target := Target{}
		data, err := os.ReadFile(fmt.Sprintf("%s/%s", Dirs.Source, entry.Name()))
		if err != nil {
			return nil, err
		}

		if err := json.Unmarshal(data, &target); err != nil {
			return nil, err
		}

		if targetName != target.Name {
			log.Printf("target %s should have name field set to %s not %s", targetName, targetName, target.Name)
			continue
		}

		targets[target.Name] = target

	}
	return targets, nil
}
