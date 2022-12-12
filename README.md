# Ocean_animation
An repo of ocean animation aimed to test basic physics-based rendering.

Relevant reference:

**Parallax mapping** 

Ocean floor (find normal maps for “sandy floor”)

- https://learnopengl.com/Advanced-Lighting/Parallax-Mapping

- https://learnopengl.com/Advanced-Lighting/Normal-Mapping

- https://www.indigorenderer.com/materials/categories/1?sort=date

**Caustics**

- https://medium.com/@evanwallace/rendering-realtime-caustics-in-webgl-2a99a29a0b2c

- https://www.deepsea.com/wp-content/uploads/2022/01/201305_Understading_Basics_Underwater_Lighting_ONT.pdf

- http://www.graphics.stanford.edu/courses/cs348b-competition/cs348b-16/second_report.pdf

 

**Extras**

Procedurally generated water (Animated)

- https://jklintan.github.io/html/proceduralWater.html

- Trochoidal wave (given position on plane, find displacement; vertex shader)

- Noise displacement (sample from texture) 

- Combine with caustics to determine underwater illumination

 

Color attenuation (fogging?)

This project has following dependencies:

- glm
- eigen
- imgui
- glad and glfw

All dependencies are self-served, so one would only needs to use this repo and run the code.
