package make

import (
	"DirkBuildTool/config"
	"DirkBuildTool/models"
	"bytes"
	"embed"
	"fmt"
	"log"
	"os"
	"os/exec"
)

type Makefile interface {
	toBytes() []byte
	getModuleName() string
	getModulePath() string
}

func RunMakefile(makefile Makefile) error {
	intDir := fmt.Sprintf("%s/%s", config.Dirs.Intermediate, makefile.getModuleName())
	if err := os.MkdirAll(intDir, config.DirPerm); err != nil {
		return err
	}

	path := fmt.Sprintf("%s/Makefile", intDir)
	if err := os.WriteFile(path, makefile.toBytes(), config.FilePerm); err != nil {
		return err
	}

	cmd := exec.Command("make", "-f", path, "-j", "8")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Dir = makefile.getModulePath()

	if err := cmd.Run(); err != nil {
		fmt.Printf("There was an error building %s, see logs for details\n", makefile.getModuleName())
		return err
	}

	log.Printf("Successfully built %s", makefile.getModuleName())
	return nil
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
	Name, Path, RootDir string
	BuildMode           *models.BuildMode
	IncDirs, Libs       []string
	Defines             map[string]string
	LdFlags, CFlags     []string
	IsLib, IsStatic     bool
	Optimize            bool
}

func (m *CppMakefile) toBytes() []byte {
	buffer := bytes.NewBuffer(nil)

	buffer.WriteString("TARGET=")
	if m.IsLib {
		buffer.WriteString("lib")
	}
	buffer.WriteString(m.Name)
	if m.IsLib {
		if m.IsStatic {
			buffer.WriteString(".a")
		} else {
			buffer.WriteString(".so")
		}
	}
	newLine(buffer)

	writeVar(buffer, "INT_DIR", fmt.Sprintf("%s/%s/%s", config.Dirs.Intermediate, m.getModuleName(), m.BuildMode.Name))
	writeVar(buffer, "BIN_DIR", config.Dirs.Binaries)
	writeVar(buffer, "CFLAGS", m.CFlags...)

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

	writeVar(buffer, "LDFLAGS", m.LdFlags...)

	ldLibs := []string{}
	for _, lib := range m.Libs {
		ldLibs = append(ldLibs, fmt.Sprintf("-l%s", lib))
	}
	writeVar(buffer, "LDLIBS", ldLibs...)
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

	return buffer.Bytes()
}

func (m *CppMakefile) getModuleName() string { return m.Name }
func (m *CppMakefile) getModulePath() string { return m.Path }

type ShaderMakefile struct {
	Name    string
	Path    string
	RootDir string
}

func (m *ShaderMakefile) toBytes() []byte {
	buffer := bytes.NewBuffer(nil)
	writeVar(buffer, "INT_DIR", fmt.Sprintf("%s/%s", config.Dirs.Intermediate, m.getModuleName()))
	writeVar(buffer, "SHADER_DIR", m.Path)
	writeBase(buffer, "shaders")
	return buffer.Bytes()
}

func (m *ShaderMakefile) getModuleName() string { return m.Name }
func (m *ShaderMakefile) getModulePath() string { return m.Path }
