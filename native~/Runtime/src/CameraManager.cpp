#include "CameraManager.h"

#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Transforms.h>
#include <CesiumUtility/Math.h>

#include <DotNet/CesiumForUnity/CesiumGeoreference.h>
#include <DotNet/UnityEngine/Camera.h>
#include <DotNet/UnityEngine/GameObject.h>
#include <DotNet/UnityEngine/Transform.h>
#include <DotNet/UnityEngine/Vector3.h>
#include <glm/trigonometric.hpp>

#if UNITY_EDITOR
#include <DotNet/UnityEditor/SceneView.h>
#endif

using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace DotNet::UnityEngine;
using namespace DotNet::CesiumForUnity;

#if UNITY_EDITOR
using namespace DotNet::UnityEditor;
#endif

namespace CesiumForUnityNative {

namespace {

ViewState unityCameraToViewState(
    const LocalHorizontalCoordinateSystem* pCoordinateSystem,
    Camera& camera) {
  Transform transform = camera.transform();

  Vector3 cameraPositionUnity = transform.position();
  glm::dvec3 cameraPosition(
      cameraPositionUnity.x,
      cameraPositionUnity.y,
      cameraPositionUnity.z);

  Vector3 cameraDirectionUnity = transform.forward();
  glm::dvec3 cameraDirection = glm::dvec3(
      cameraDirectionUnity.x,
      cameraDirectionUnity.y,
      cameraDirectionUnity.z);

  Vector3 cameraUpUnity = transform.up();
  glm::dvec3 cameraUp =
      glm::dvec3(cameraUpUnity.x, cameraUpUnity.y, cameraUpUnity.z);

  if (pCoordinateSystem) {
    cameraPosition = pCoordinateSystem->localPositionToEcef(cameraPosition);
    cameraDirection = pCoordinateSystem->localDirectionToEcef(cameraDirection);
    cameraUp = pCoordinateSystem->localDirectionToEcef(cameraUp);
  }

  double verticalFOV = Math::degreesToRadians(camera.fieldOfView());
  double horizontalFOV =
      2 * glm::atan(camera.aspect() * glm::tan(verticalFOV * 0.5));

  return ViewState::create(
      cameraPosition,
      cameraDirection,
      cameraUp,
      glm::dvec2(camera.pixelWidth(), camera.pixelHeight()),
      horizontalFOV,
      verticalFOV);
}

} // namespace

std::vector<ViewState> CameraManager::getAllCameras(const GameObject& context) {
  const LocalHorizontalCoordinateSystem* pCoordinateSystem = nullptr;

  CesiumGeoreference georeferenceComponent =
      context.GetComponentInParent<CesiumGeoreference>();
  if (georeferenceComponent != nullptr) {
    CesiumGeoreferenceImpl& georeference =
        georeferenceComponent.NativeImplementation();
    pCoordinateSystem = &georeference.getCoordinateSystem();
  }

  std::vector<ViewState> result;
  Camera camera = Camera::main();
  if (camera != nullptr) {
    result.emplace_back(unityCameraToViewState(pCoordinateSystem, camera));
  }

#if UNITY_EDITOR
  SceneView lastActiveEditorView = SceneView::lastActiveSceneView();
  if (lastActiveEditorView != nullptr) {
    Camera editorCamera = lastActiveEditorView.camera();
    if (camera != nullptr) {
      result.emplace_back(
          unityCameraToViewState(pCoordinateSystem, editorCamera));
    }
  }
#endif

  return result;
}

} // namespace CesiumForUnityNative