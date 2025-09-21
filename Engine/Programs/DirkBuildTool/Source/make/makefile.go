package make

import "embed"

type Makefile interface {
	ToBytes() ([]byte, error)
	writeVar(key string, values ...string)
	newLine()
	writeBase(name string)
}

//go:embed makefiles
var makefiles embed.FS
