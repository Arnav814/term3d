#ifndef SCENE_HPP
#define SCENE_HPP

#include "renderable.hpp"
#include "structures.hpp"
#include "../drawing/setColor.hpp"

#include <memory>

struct Scene {
	std::vector<std::shared_ptr<Object3D>>
	    objects; // TODO: do I need this if it's all pointed to by instances?
	std::vector<InstanceRef3D> instances;
	std::vector<std::shared_ptr<Light>> lights;
	Camera camera;
	Color bgColor;
	double ambientLight;
};

[[nodiscard]] Scene initScene();

#endif /* SCENE_HPP */
