package build

import (
	"DirkBuildTool/config"
	"DirkBuildTool/module"
	"errors"
	"fmt"
)

type Graph struct {
	dependents map[module.Module][]module.Module
	counts     map[module.Module]int
	nodes      map[module.Module]bool
	modules    map[string]module.Module
}

func (g *Graph) addDependency(task, dep module.Module) {
	g.nodes[task] = true
	g.nodes[dep] = true

	g.dependents[dep] = append(g.dependents[dep], task)

	g.counts[task]++
}

func (g *Graph) flatten() ([]module.Module, error) {
	var queue []module.Module
	var result []module.Module

	for node := range g.nodes {
		if g.counts[node] == 0 {
			queue = append(queue, node)
		}
	}

	for len(queue) > 0 {
		current := queue[0]
		queue = queue[1:]

		result = append(result, current)

		for _, dependent := range g.dependents[current] {
			g.counts[dependent]--

			if g.counts[dependent] == 0 {
				queue = append(queue, dependent)
			}
		}
	}

	// TODO: actually find out which dependencies are circular
	if len(result) != len(g.nodes) {
		return nil, errors.New("circular dependency detected: graph cannot be flattened")
	}

	return result, nil
}

func (g *Graph) addModuleDependencies(module module.Module) error {
	for _, depName := range module.GetDependencies() {
		dep, ok := g.modules[depName]
		if !ok {
			return fmt.Errorf("module %s depended on by %s does not exist", depName, module.GetName())
		}

		module.AddDependency(dep)
		g.addDependency(module, dep)
		if err := g.addModuleDependencies(dep); err != nil {
			return err
		}
	}

	return nil
}

func buildDependencyGraph(modules map[string]module.Module, target config.Target) ([]module.Module, error) {
	g := Graph{
		dependents: map[module.Module][]module.Module{},
		counts:     map[module.Module]int{},
		nodes:      map[module.Module]bool{},
		modules:    modules,
	}

	for _, moduleName := range target.Modules {
		module, ok := modules[moduleName]
		if !ok {
			return nil, fmt.Errorf("module %s specified by target %s does not exist", moduleName, target.Name)
		}

		g.addModuleDependencies(module)
	}

	return g.flatten()
}
