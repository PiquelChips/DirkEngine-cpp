package config

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
)

type BuildType struct {
	Name     string   `json:"-"`
	Optimize bool     `json:"optimize"`
	Compact  bool     `json:"compact"` // compact the output (essentially statically linking)
	Defines  []string `json:"defines"`
}

type DirsConfig struct {
	Root         string   `json:"-"`
	Intermediate string   `json:"intermediate"`
	Binaries     string   `json:"binaries"`
	Thirdparty   string   `json:"thirdparty"`
	Modules      []string `json:"modules"` // dirs that will be searched for modules
}

type Config struct {
	BuildTypes map[string]*BuildType
	Dirs       DirsConfig
}

var config *Config
const configDir = "Engine/Programs/DirkBuildTool/Config"

func Get() *Config {
	if config == nil {
		LoadConfig()
	}
	return config
}

func LoadConfig() {
	log.Printf("Loading configuration\n")
	config = &Config{}
	// dirs
	if err := loadConfig("dirs.json", &config.Dirs); err != nil {
		fmt.Printf("%s\n", err.Error())
		os.Exit(1)
		return
	}

	config.Dirs.Root, _ = filepath.Abs(".")
	config.Dirs.Intermediate, _ = filepath.Abs(config.Dirs.Intermediate)
	config.Dirs.Binaries, _ = filepath.Abs(config.Dirs.Binaries)
	config.Dirs.Thirdparty, _ = filepath.Abs(config.Dirs.Thirdparty)
	for i, module := range config.Dirs.Modules {
		config.Dirs.Modules[i], _ = filepath.Abs(module)
	}

	const DirPerm = 0755

	if err := os.MkdirAll(config.Dirs.Intermediate, DirPerm); err != nil {
		panic(err)
	}
	if err := os.MkdirAll(config.Dirs.Binaries, DirPerm); err != nil {
		panic(err)
	}

	// build types
	if err := loadConfig("build_configs.json", &config.BuildTypes); err != nil {
		fmt.Printf("%s\n", err.Error())
		os.Exit(1)
		return
	}

	if len(config.BuildTypes) < 1 {
		fmt.Printf("Config must have at least one build type\n")
		os.Exit(1)
		return
	}

	for name, conf := range config.BuildTypes {
		conf.Name = name
	}
}

func loadConfig(file string, out any) error {
	data, err := os.ReadFile(fmt.Sprintf("%s/%s", configDir, file))
	if err != nil {
		return fmt.Errorf("Error loading config file: %w", err)
	}

	if err := json.Unmarshal(data, out); err != nil {
		return fmt.Errorf("Error parsing config file: %w", err)
	}

	return nil
}
