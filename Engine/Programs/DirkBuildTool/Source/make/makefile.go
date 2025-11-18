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

type CppMakefile struct {
	Name, Target       string
	BuildType, RootDir string
	IncDirs            []string
	Libs               []string
	Defines            map[string]string
	LdFlags, CFlags    string
	IsLib, IsStatic    bool
	Optimize           bool
}

func (m *CppMakefile) ToBytes() ([]byte, error) {
	buffer := bytes.NewBuffer(nil)

	writeVar(buffer, "NAME", m.Name)

	buffer.WriteString("TARGET=")
	if m.IsLib {
		buffer.WriteString("lib")
	}
	buffer.WriteString(m.Target)
	if m.IsLib {
		if m.IsStatic {
			buffer.WriteString(".a")
		} else {
			buffer.WriteString(".so")
		}
	}
	newLine(buffer)

	writeVar(buffer, "ROOT_DIR", m.RootDir)
	writeVar(buffer, "BUILD_TYPE", m.BuildType)
	writeVar(buffer, "CFLAGS", m.CFlags)

	if m.Optimize {
		buffer.WriteString("CFLAGS+= -O3\n")
	}

	cxxFlags := []string{}
	for _, dir := range m.IncDirs {
		cxxFlags = append(cxxFlags, fmt.Sprintf("-I%s", dir))
	}
	writeVar(buffer, "CXXFLAGS", cxxFlags...)

	defines := []string{}
	for key, value := range m.Defines {
		if value == "" {
			defines = append(defines, fmt.Sprintf("-D%s", key))
		} else {
			defines = append(defines, fmt.Sprintf("-D%s=\\\"%s\\\"", key, value))
		}
	}
	writeVar(buffer, "DEFINES", defines...)

	writeVar(buffer, "LDFLAGS", m.LdFlags)

	ldLibs := []string{}
	for _, lib := range m.Libs {
		ldLibs = append(ldLibs, fmt.Sprintf("-l%s", lib))
	}
	writeVar(buffer, "LDLIBS", ldLibs...)

	buffer.WriteString("LDFLAGS+= -g\n")
	buffer.WriteString("CXXFLAGS+= -g\n")

	writeBase(buffer, "base")
	writeBase(buffer, "compilation")

	if !m.IsLib && !m.IsStatic {
		// exec
		writeBase(buffer, "cxx_lnk")
	} else if !m.IsLib && m.IsStatic {
		// static exec
		writeBase(buffer, "cxx_lnk")
	} else if m.IsLib && !m.IsStatic {
		// shared lib
		buffer.WriteString("LDFLAGS+= -shared\n")
		writeBase(buffer, "cxx_lnk")
	} else if m.IsLib && m.IsStatic {
		// static lib
		writeBase(buffer, "ar_lnk")
	}

	return buffer.Bytes(), nil
}

type ShaderMakefile struct {
	Name    string
	Path    string
	RootDir string
}

func (m *ShaderMakefile) ToBytes() ([]byte, error) {
	buffer := bytes.NewBuffer(nil)
	writeVar(buffer, "NAME", m.Name)
	writeVar(buffer, "ROOT_DIR", m.RootDir)
	writeVar(buffer, "SHADER_DIR", m.Path)
	writeBase(buffer, "shaders")
	return buffer.Bytes(), nil
}
