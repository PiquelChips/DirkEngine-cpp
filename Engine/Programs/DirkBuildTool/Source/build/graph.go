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

func (g *Graph) getNodes() map[string]module.Module {
	return g.nodes
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

	if len(result) != len(g.nodes) {
		return nil, g.findCycle()
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
	if err := g.findCycle(); err != nil {
		return Graph{}, err
	}

	required := map[string]module.Module{}

	for _, target := range targets {
		newReq := g.getDependencies(target)
		if newReq != nil {
			maps.Copy(required, newReq)
		}
	}

	return buildDependencyGraph(required)
}

func (g *Graph) findCycle() error {
	unproccessed := map[module.Module]bool{}
	for node, count := range g.counts {
		if count > 0 {
			unproccessed[node] = true
		}
	}

	var path []module.Module
	visited := map[module.Module]bool{}

	var search func(current module.Module) error
	search = func(current module.Module) error {
		visited[current] = true
		path = append(path, current)

		// Iterate over what depends on the 'current' module
		for _, dependent := range g.dependents[current] {
			// Only traverse nodes that are part of the unproccessed stuck set
			if !unproccessed[dependent] {
				continue
			}

			// check if 'dependent' is already in the current path (cycle detected)
			if idx := slices.Index(path, dependent); idx != -1 {
				cycle := path[idx:]
				msg := "circular dependency detected: "
				for i, mod := range cycle {
					if i > 0 {
						msg += " -> "
					}
					msg += mod.GetName()
				}
				msg += " -> " + dependent.GetName()
				return errors.New(msg)
			}

			if !visited[dependent] {
				if err := search(dependent); err != nil {
					return err
				}
			}
		}

		// remove current node from path before returning (going back up a node)
		path = path[:len(path)-1]
		return nil
	}

	for node := range unproccessed {
		if !visited[node] {
			if err := search(node); err != nil {
				return err
			}
		}
	}

	return nil
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
