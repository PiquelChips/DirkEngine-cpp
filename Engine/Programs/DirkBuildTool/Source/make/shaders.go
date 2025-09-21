package make

import "bytes"

type ShaderMakefile struct {
	Path    string
	RootDir string
	buffer  *bytes.Buffer
}

func (m *ShaderMakefile) ToBytes() ([]byte, error) {
	m.buffer = bytes.NewBuffer(nil)
	writeVar(m.buffer, "ROOT_DIR", m.RootDir)
	writeVar(m.buffer, "SHADER_DIR", m.Path)
	writeBase(m.buffer, "shaders")
	return m.buffer.Bytes(), nil
}
