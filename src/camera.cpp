#include "camera.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

glm::mat4 Camera::getView() { return view; }

glm::vec2 Camera::rot() { return _rot; }
void Camera::rot(glm::vec2 rot) {
  _rot = rot;
  updateView();
}

glm::vec3 Camera::pos() { return _pos; }
void Camera::pos(glm::vec3 pos) {
  _pos = pos;
  updateView();
}

glm::vec3 Camera::up() { return _up; }
void Camera::up(glm::vec3 up) { _up = up; }

glm::vec3 Camera::front() { return _front; }
void Camera::front(glm::vec3 front) {
  _front = front;
  updateView();
}

void Camera::lookAt(glm::vec3 look) { view = glm::lookAt(_pos, look, _up); }

void Camera::updateView() { view = glm::lookAt(_pos, _pos + _front, _up); }

glm::mat4 CameraPerpective::getProj() { return proj; }

void CameraPerpective::setProj(float fov, float aspect, float xNear,
                               float xFar) {
  proj = glm::perspective(fov, aspect, xNear, xFar);
}

glm::mat4 CameraOrtho::getProj() { return proj; }

void CameraOrtho::setProj(float xMin, float xMax, float yMin, float yMax,
                          float Zmin, float Zmax) {
  proj = glm::ortho(xMin, xMax, yMin, yMax, Zmin, Zmax);
}
