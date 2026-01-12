package config

import (
	"encoding/json"
	"fmt"
	"os"
	"strings"
)

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
