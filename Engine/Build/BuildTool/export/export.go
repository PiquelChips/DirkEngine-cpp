package export

import (
	"os"
)

var EngineRoot = os.Getenv("PWD")

// this type handles exporting to various build systems
type Exporter interface {
	Export() error
}
