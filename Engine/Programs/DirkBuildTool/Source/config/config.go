package config

import (
	"DirkBuildTool/output"
	"encoding/json"
	"fmt"
	"log"
	"os"
)

type BuildType struct {
	Name     string   `json:"-"`
	Optimize bool     `json:"opmtimize"`
	Shipping bool     `json:"shipping"`
	Defines  []string `json:"defines"`
}

type Config struct {
	BuildTypes map[string]*BuildType
}

func LoadConfig() *Config {
	log.Printf("Loading configuration")
	data, err := os.ReadFile(fmt.Sprintf("%s/build_configs.json", output.Dirs.Config))
	if err != nil {
		fmt.Printf("Error loading config file: %s\n", err.Error())
		return nil
	}

	config := &Config{}
	if err := json.Unmarshal(data, &config.BuildTypes); err != nil {
		fmt.Printf("Error parsing config file: %s\n", err.Error())
		return nil
	}

	if len(config.BuildTypes) < 1 {
		fmt.Printf("Config must have at least one build type\n")
		return nil
	}

	for name, conf := range config.BuildTypes {
		conf.Name = name
	}

	return config
}
