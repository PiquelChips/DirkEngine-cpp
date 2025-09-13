package output

import (
	"fmt"
	"os"
	"strings"
)

const filePerm = 0644

func WriteIntFile(name string, data []byte) error {
	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", Dirs.Intermediate, name)
	return os.WriteFile(name, data, filePerm)
}

func ReadIntFile(name string) ([]byte, error) {
	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", Dirs.Intermediate, name)
	return os.ReadFile(name)
}
