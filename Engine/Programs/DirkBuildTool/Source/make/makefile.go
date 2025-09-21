package make

import (
	"bytes"
	"embed"
	"fmt"
)

type Makefile interface {
	ToBytes() ([]byte, error)
}

//go:embed makefiles
var makefiles embed.FS

func writeVar(buffer *bytes.Buffer, key string, values ...string) {
	buffer.WriteString(key)
	buffer.WriteString("=")
	if len(values) == 1 {
		buffer.WriteString(values[0])
	} else {
		for _, value := range values {
			buffer.WriteString(value)
			buffer.WriteString(" ")
		}
	}
	newLine(buffer)
}

func newLine(buffer *bytes.Buffer) { buffer.WriteString("\n") }

func writeBase(buffer *bytes.Buffer, name string) {
	data, err := makefiles.ReadFile(fmt.Sprintf("makefiles/%s.make", name))
	if err != nil {
		panic(err)
	}
	buffer.Write(data)
	newLine(buffer)
}
