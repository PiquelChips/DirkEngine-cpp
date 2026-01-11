package build

import (
	"DirkBuildTool/module"
	"errors"
	"fmt"
	"maps"
	"slices"
)

type Graph struct {
	dependents map[module.Module][]module.Module
	counts     map[module.Module]int
	nodes      map[string]module.Module
}

func (g *Graph) addDependency(task, dep module.Module) {
	g.nodes[task.GetName()] = task
	g.nodes[dep.GetName()] = dep

	if slices.Contains(g.dependents[dep], task) {
		return
	}

	g.dependents[dep] = append(g.dependents[dep], task)
	g.counts[task]++
}

func (g *Graph) flatten() ([]module.Module, error) {
	var queue []module.Module
	var result []module.Module

	for _, node := range g.nodes {
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

func (g *Graph) getDependencies(mod module.Module) map[string]module.Module {
	required := map[string]module.Module{mod.GetName(): mod}

	for _, depName := range mod.GetDependencies() {
		dep := g.nodes[depName]

		newReq := g.getDependencies(dep)
		if newReq != nil {
			maps.Copy(required, newReq)
		}
	}

	return required
}

func (g *Graph) strip(targets []module.Module) (Graph, error) {
	if _, err := g.flatten(); err != nil {
		return Graph{}, err
	}

	required := map[string]module.Module{}

	for _, target := range targets {
		newReq := g.getRequired(target)
		if newReq != nil {
			maps.Copy(required, newReq)
		}
	}

	return buildDependencyGraph(required)
}

func buildDependencyGraph(modules map[string]module.Module) (Graph, error) {
	g := Graph{
		dependents: map[module.Module][]module.Module{},
		counts:     map[module.Module]int{},
		nodes:      map[string]module.Module{},
	}

	for moduleName, module := range modules {
		for _, depName := range module.GetDependencies() {
			dep, ok := modules[depName]
			if !ok {
				return Graph{}, fmt.Errorf("module %s required by %s does not exist", depName, moduleName)
			}

			g.addDependency(module, dep)
			module.AddDependency(dep)
		}
	}

	return g, nil
}
