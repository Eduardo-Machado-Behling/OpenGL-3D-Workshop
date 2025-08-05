#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "glm/fwd.hpp"
#include <glm/glm.hpp>

struct Camera {
  glm::mat4 getView();

  glm::vec2 rot();
  void rot(glm::vec2);

  glm::vec3 pos();
  void pos(glm::vec3);

  glm::vec3 up();
  void up(glm::vec3);

  glm::vec3 front();
  void front(glm::vec3);

  void lookAt(glm::vec3);

  virtual glm::mat4 getProj() = 0;

private:
  void updateView();

  glm::vec3 _up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 _front = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 _pos = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec2 _rot = glm::vec2(-90, 0);
  glm::mat4 view;
};

struct CameraPerpective : public Camera {
  glm::mat4 getProj() override;

  void setProj(float fov, float aspect, float xNear, float xFar);

  float fov;
  float aspect;
  glm::vec2 x;

private:
  glm::mat4 proj;
};

struct CameraOrtho : public Camera {
  glm::mat4 getProj() override;

  void setProj(float xMin, float xMax, float yMin, float yMax, float Zmin,
               float Zmax);

  glm::vec2 x;
  glm::vec2 y;
  glm::vec2 z;

private:
  glm::mat4 proj;
};

#endif
