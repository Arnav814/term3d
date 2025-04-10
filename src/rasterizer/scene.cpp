#include "scene.hpp"

#include "shapeBuilders.hpp"

#include <glm/gtx/euler_angles.hpp>

[[nodiscard]] Scene initScene() {
	Camera camera{1, 1, 1};
	camera.setTransform(
	    Transform({-3, 1, 0}, glm::yawPitchRoll<double>(glm::radians(-30.0), 0, 0), 1.0));
	// camera.setTransform(
	//     Transform({9, 0, 0}, glm::identity<glm::dmat3>(), 1.0));

	double ambientLight = 0.2;
	Scene scene{{}, {}, {}, camera, Color(Category(true, 7), RGBA(0, 0, 0, 255)), ambientLight};

	Object3D cube(
	    {
	        {1,  1,  1 },
	        {-1, 1,  1 },
	        {-1, -1, 1 },
	        {1,  -1, 1 },
	        {1,  1,  -1},
	        {-1, 1,  -1},
	        {-1, -1, -1},
	        {1,  -1, -1}
    },
	    {
	        {{0, 1, 2}, cred, {dvec3{0, 0, 1}, dvec3{0, 0, 1}, dvec3{0, 0, 1}}},
	        {{0, 2, 3}, cred, {dvec3{0, 0, 1}, dvec3{0, 0, 1}, dvec3{0, 0, 1}}},
	        {{4, 0, 3}, cgreen, {dvec3{1, 0, 0}, dvec3{1, 0, 0}, dvec3{1, 0, 0}}},
	        {{4, 3, 7}, cgreen, {dvec3{1, 0, 0}, dvec3{1, 0, 0}, dvec3{1, 0, 0}}},
	        {{5, 4, 7}, cblue, {dvec3{0, 0, -1}, dvec3{0, 0, -1}, dvec3{0, 0, -1}}},
	        {{5, 7, 6}, cblue, {dvec3{0, 0, -1}, dvec3{0, 0, -1}, dvec3{0, 0, -1}}},
	        {{1, 5, 6}, cyellow, {dvec3{-1, 0, 0}, dvec3{-1, 0, 0}, dvec3{-1, 0, 0}}},
	        {{1, 6, 2}, cyellow, {dvec3{-1, 0, 0}, dvec3{-1, 0, 0}, dvec3{-1, 0, 0}}},
	        {{4, 5, 1}, cmagenta, {dvec3{0, 1, 0}, dvec3{0, 1, 0}, dvec3{0, 1, 0}}},
	        {{4, 1, 0}, cmagenta, {dvec3{0, 1, 0}, dvec3{0, 1, 0}, dvec3{0, 1, 0}}},
	        {{2, 6, 7}, ccyan, {dvec3{0, -1, 0}, dvec3{0, -1, 0}, dvec3{0, -1, 0}}},
	        {{2, 7, 3}, ccyan, {dvec3{0, -1, 0}, dvec3{0, -1, 0}, dvec3{0, -1, 0}}},
	    },
	    3);
	scene.objects.push_back(std::make_shared<Object3D>(cube));

	// Object3D sphere = makeSphere(ccyan, -1, 1.0, 1);
	// scene.objects.push_back(std::make_shared<Object3D>(sphere));

	scene.instances.push_back(InstanceRef3D(std::make_shared<Object3D>(cube),
	                                        {
	                                            {-1.5, 0, 7},
	                                            // glm::identity<dmat4>(),
	                                            glm::yawPitchRoll<double>(glm::radians(90.0), 0, 0),
	                                            0.75
    }));

	Object3D axes{{}, {}, -1}; // helpful visualization
	makePyramid(axes, cred, origin, {3, 0, 0}, {0, 0.5, 0}); // x axis
	makePyramid(axes, cgreen, origin, {0, 3, 0}, {0.5, 0, 0}); // y axis
	makePyramid(axes, cblue, origin, {0, 0, 3}, {0, 0.5, 0}); // z axis
	scene.objects.push_back(std::make_shared<Object3D>(axes));

	scene.instances.push_back(InstanceRef3D(std::make_shared<Object3D>(axes), {}));

	scene.instances.push_back(InstanceRef3D(
	    std::make_shared<Object3D>(cube), {
	                                          {1.25, 2.5, 7.5},
	                                          // glm::identity<dmat4>(),
	                                          glm::yawPitchRoll<double>(glm::radians(195.0), 0, 0),
	                                          1.0
    }));

	// scene.instances.push_back(
	//     InstanceRef3D(std::make_shared<Object3D>(sphere),
	//                   {
	//                       {2, 0, 7},
	// glm::yawPitchRoll<double>(0, 0, 0), 1.0
	// }));

	scene.lights.push_back(std::make_shared<DirectionalLight>(0.3, dvec3(1.0, 4.0, 4.0)));
	scene.lights.push_back(std::make_shared<PointLight>(0.6, dvec3(2.0, 1.0, 0.0)));
	// scene.lights.push_back(std::make_shared<PointLight>(2, dvec3(2.0, 1.0, 0.0)));
	// scene.lights.push_back(std::make_shared<DirectionalLight>(0.8, dvec3(1.0, 4.0, 4.0)));

	return scene;
}
