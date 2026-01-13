package config

const (
	PlatformLinux = "Linux"
)

func detectPlatform() PlatformConfig {
	return PlatformConfig{
		Name:    PlatformLinux,
		Defines: map[string]string{"PLATFORM_LINUX": ""},
	}
}
