package refrules

fruit.apple.seeds = 12
fruit.orange.colors = {"orange"}
fruit.apple["name"] = "apple"
fruit.apple.colors = {"red", "green"}
fruit["orange"].size = 5.3
fruit.orange.name = "orange"
fruit["apple"]["color.count"] = count(fruit.apple["colors"])
fruit["color.name"](fruit, color) := name {
	fruit.colors[color]
  name := concat(" ", [color, fruit.name])
}

output := {
	"x": data.refrules.fruit,
  "y": data.refrules.fruit["color.name"](data.refrules.fruit.apple, "green")
}