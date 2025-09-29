package output

import (
	"DirkBuildTool/config"
	"fmt"
	"os"
	"strings"
)

const DirPerm = 0755
const FilePerm = 0644

func WriteIntFile(name string, data []byte, overwrite bool) error {
	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", config.Dirs.Intermediate, name)

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

func ReadIntFile(name string) ([]byte, error) {
	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", config.Dirs.Intermediate, name)
	return os.ReadFile(name)
}

func GetIntFileInfo(name string) (os.FileInfo, error) {
	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", config.Dirs.Intermediate, name)

	return os.Stat(name)
}
