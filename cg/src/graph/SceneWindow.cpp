//[]---------------------------------------------------------------[]
//|                                                                 |
//| Copyright (C) 2020, 2022 Paulo Pagliosa.                        |
//|                                                                 |
//| This software is provided 'as-is', without any express or       |
//| implied warranty. In no event will the authors be held liable   |
//| for any damages arising from the use of this software.          |
//|                                                                 |
//| Permission is granted to anyone to use this software for any    |
//| purpose, including commercial applications, and to alter it and |
//| redistribute it freely, subject to the following restrictions:  |
//|                                                                 |
//| 1. The origin of this software must not be misrepresented; you  |
//| must not claim that you wrote the original software. If you use |
//| this software in a product, an acknowledgment in the product    |
//| documentation would be appreciated but is not required.         |
//|                                                                 |
//| 2. Altered source versions must be plainly marked as such, and  |
//| must not be misrepresented as being the original software.      |
//|                                                                 |
//| 3. This notice may not be removed or altered from any source    |
//| distribution.                                                   |
//|                                                                 |
//[]---------------------------------------------------------------[]
//
// OVERVIEW: SceneWindowBase.cpp
// ========
// Source file for generic graph scene window.
//
// Author: Paulo Pagliosa
// Last revision: 26/02/2022

#include "graph/SceneWindow.h"
#include "graphics/Assets.h"
#include <cassert>

namespace cg
{ // begin namespace cg

namespace graph
{ // begin namespace graph


/////////////////////////////////////////////////////////////////////
//
// SceneWindow implementation
// ===========
SceneBase*
SceneWindow::makeScene()
{
  auto scene = Scene::New();

  SceneObjectBuilder::setScene(*scene);
  _currentNode = scene;
  return scene;
}

void
SceneWindow::setScene(Scene& scene)
{
  SceneObjectBuilder::setScene(scene);
  _currentNode = &scene;
  editor()->setScene(scene);
  _viewMode = ViewMode::Editor;
}

void
SceneWindow::renderScene()
{
  // do nothing
}

void
SceneWindow::drawComponents(const SceneObject& object)
{
  auto editor = this->editor();
  const auto& components = object.components();

  // Iterate the object components skipping its transform
  for (auto end = components.end(), cit = ++components.begin(); cit != end;)
  {
    Component* c{*cit++};

    if (auto proxy = graph::asLight(c))
      editor->drawLight(*proxy->light());
    else if (auto proxy = graph::asCamera(c))
    {
      editor->drawCamera(*proxy->camera());
      preview(*proxy->camera());
    }
    else if (auto proxy = graph::asPrimitive(c))
    {
      auto p = proxy->mapper()->primitive();

      assert(p != nullptr);
      if (auto mesh = p->tesselate())
      {
        editor->setPolygonMode(GLGraphics3::LINE);
        editor->setMeshColor(_selectedWireframeColor);
        editor->drawMesh(*mesh, p->localToWorldMatrix(), p->normalMatrix());
      }
    }
  }
}

void
SceneWindow::drawSelectedObject(const SceneObject& object)
{
  if (!object.visible())
    return;
  drawComponents(object);
  for (auto& child : object.children())
    drawSelectedObject(*child);
}

void
SceneWindow::render()
{
  if (_viewMode != ViewMode::Editor)
  {
    renderScene();
    return;
  }
  SceneWindowBase::render();
  if (auto object = _currentNode.as<SceneObject>())
  {
    const auto t = object->transform();

    drawSelectedObject(*object);
    editor()->drawTransform(t->position(), t->rotation());
  }
}

void
SceneWindow::createObjectMenu()
{
  // do nothing
}

inline void
SceneWindow::createObjectButton()
{
  if (ImGui::Button("Create###CreateObjectButton"))
    ImGui::OpenPopup("CreateObjectPopup");
  if (ImGui::BeginPopup("CreateObjectPopup"))
  {
    if (ImGui::MenuItem("Empty Object"))
      createEmptyObject();
    ImGui::Separator();
    createObjectMenu();
    if (ImGui::BeginMenu("Light"))
    {
      if (ImGui::MenuItem("Directional Light"))
        createLightObject(Light::Type::Directional);
      if (ImGui::MenuItem("Point Light"))
        createLightObject(Light::Type::Point);
      if (ImGui::MenuItem("Spotlight"))
        createLightObject(Light::Type::Spot);
      ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Camera"))
      createCameraObject();
    ImGui::EndPopup();
  }
}

inline bool
SceneWindow::treeNode(SceneNode node, ImGuiTreeNodeFlags flags)
{
  if (node == _currentNode)
    flags |= ImGuiTreeNodeFlags_Selected;

  auto open = ImGui::TreeNodeEx(node, flags, node->name());

  if (ImGui::IsItemClicked())
    _currentNode = node;
  return open;
}

inline bool
SceneWindow::deleteObjectPopup(SceneObject& object)
{
  auto deleted = false;

  if (ImGui::BeginPopupContextItem())
  {
    if (ImGui::MenuItem("Delete###DeleteObject"))
    {
      auto parent = object.parent();

      if (parent->parent() == nullptr)
        _currentNode = parent->scene();
      else
        _currentNode = parent;
      parent->removeChild(&object);
      deleted = true;
    }
    ImGui::EndPopup();
  }
  return deleted;
}

namespace
{ // begin namespace

bool
dropSceneObject(SceneObject& target)
{
  if (ImGui::BeginDragDropTarget() && target.movable())
  {
    if (auto* payload = ImGui::AcceptDragDropPayload("SceneObject"))
    {
      auto object = *(SceneObject**)payload->Data;

      if (&target != object)
      {
        object->setParent(&target);
        return true;
      }
    }
    ImGui::EndDragDropTarget();
  }
  return false;
}

} // end namespace

bool
SceneWindow::objectHierarchy(SceneObject& object)
{
  for (auto& child : object.children())
  {
    ImGuiTreeNodeFlags flags{ImGuiTreeNodeFlags_OpenOnArrow};
    auto hasChildren = child.childrenCount() > 0;

    if (!hasChildren)
      flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    auto open = treeNode(&child, flags);
    auto movable = child.movable() && !deleteObjectPopup(child);

    if (movable && ImGui::BeginDragDropSource())
    {
      auto* data = &child;

      ImGui::Text(child.name());
      ImGui::SetDragDropPayload("SceneObject", &data, sizeof(SceneObject*));
      ImGui::EndDragDropSource();
    }

    auto dropped = dropSceneObject(child);

    if (open)
    {
      if (movable)
        dropped |= objectHierarchy(child);
      if (hasChildren)
        ImGui::TreePop();
    }
    if (dropped)
      return true;
  }
  return false;
}

void
SceneWindow::hierarchyWindow(const char* title)
{
  if (!_showHierarchy)
    return;
  assert(title != nullptr);
  ImGui::Begin(title);
  createObjectButton();
  ImGui::Separator();
  if (treeNode(_scene.get(), ImGuiTreeNodeFlags_OpenOnArrow))
  {
    auto root = _scene->root();

    dropSceneObject(*root);
    objectHierarchy(*root);
    ImGui::TreePop();
  }
  ImGui::End();
}

void
SceneWindow::inspectTransform(Transform& transform)
{
  vec3f temp;

  temp = transform.localPosition();
  if (ImGui::dragVec3("Position", temp))
    transform.setLocalPosition(temp);
  temp = transform.localEulerAngles();
  if (ImGui::dragVec3("Rotation", temp))
    transform.setLocalEulerAngles(temp);
  temp = transform.localScale();
  if (ImGui::dragVec3("Scale", temp, 0.001f, math::Limits<float>::inf()))
    transform.setLocalScale(temp);
}

void
SceneWindow::inspectComponent(Component& component)
{
  auto typeName = component.typeName();
  auto notDelete{true};
  auto open = component.erasable() ?
    ImGui::CollapsingHeader(typeName, &notDelete) :
    ImGui::CollapsingHeader(typeName);

  if (!notDelete)
    component.sceneObject()->removeComponent(typeName);
  else if (open)
  {
    if (auto transform = graph::asTransform(&component))
      inspectTransform(*transform);
    else if (auto function = inspectFunction(component))
      function(*this, component);
  }
}

void
SceneWindow::inspectCamera(SceneWindow& window, CameraProxy& proxy)
{
  auto editor = window.editor();
  auto camera = proxy.camera();

  {
    auto isCurrent = camera == CameraProxy::current();

    ImGui::Checkbox("Current", &isCurrent);
    CameraProxy::setCurrent(isCurrent ? camera : nullptr);
  }
  SceneWindowBase::inspectCamera(*camera);
  if (ImGui::Button("Move to Editor View"))
  {
    editor->camera()->set(*camera);
    // Update to continue drawing in the same frame
    editor->update();
  }
  ImGui::SameLine();
  if (ImGui::Button("Set From Editor View"))
  {
    auto c = editor->camera();
    auto t = proxy.transform();

    t->setPosition(c->position());
    t->setRotation(c->rotation());
    camera->setProjection(*c);
  }
}

void
SceneWindow::inspectLight(SceneWindow&, LightProxy& proxy)
{
  auto light = proxy.light();

  SceneWindowBase::inspectLight(*light);
  light->turnOn(light->isTurnedOn() && proxy.sceneObject()->visible());
}

void
SceneWindow::inspectPrimitive(SceneWindow&, TriangleMeshProxy& proxy)
{
  ImGui::inputText("Mesh", proxy.meshName());
  if (ImGui::BeginDragDropTarget())
  {
    if (auto* payload = ImGui::AcceptDragDropPayload("TriangleMesh"))
    {
      auto mit = *(MeshMapIterator*)payload->Data;

      assert(mit->second != nullptr);
      proxy.setMesh(*mit->second, mit->first);
    }
    ImGui::EndDragDropTarget();
  }
  ImGui::SameLine();
  if (ImGui::Button("...###PrimitiveMesh"))
    ImGui::OpenPopup("PrimitiveMeshPopup");
  if (ImGui::BeginPopup("PrimitiveMeshPopup"))
  {
    if (auto& meshes = Assets::meshes(); !meshes.empty())
    {
      for (auto mit = meshes.begin(); mit != meshes.end(); ++mit)
        if (ImGui::Selectable(mit->first.c_str()))
          proxy.setMesh(*Assets::loadMesh(mit), mit->first);
    }
    ImGui::EndPopup();
  }
  ImGui::Separator();

  auto primitive = proxy.mapper()->primitive();
  auto material = primitive->material();

  ImGui::inputText("Material", material->name());
  if (ImGui::BeginDragDropTarget())
  {
    if (auto* payload = ImGui::AcceptDragDropPayload("Material"))
    {
      auto mit = *(MaterialMapIterator*)payload->Data;

      assert(mit->second != nullptr);
      primitive->setMaterial(material = mit->second);
    }
    ImGui::EndDragDropTarget();
  }
  inspectMaterial(*material);
  proxy.actor()->visible = proxy.sceneObject()->visible();
}

Component*
SceneWindow::addComponentMenu()
{
  return nullptr;
}

inline void
SceneWindow::addComponentButton(SceneObject& object)
{
  auto ok = true;

  if (ImGui::Button("Add Component"))
    ImGui::OpenPopup("AddComponentPopup");
  if (ImGui::BeginPopup("AddComponentPopup"))
  {
    auto component = addComponentMenu();

    if (ImGui::MenuItem("Light"))
      component = LightProxy::New();
    if (ImGui::MenuItem("Camera"))
      component = CameraProxy::New();
    if (nullptr != component)
      ok = object.addComponent(component);
    ImGui::EndPopup();
  }
  if (!ok)
    showErrorMessage("Unable to add this component type.");
}

inline void
SceneWindow::inspectSceneObject(SceneObject& object)
{
  addComponentButton(object);
  ImGui::Separator();
  ImGui::objectNameInput(object);
  ImGui::SameLine();

  bool state{object.visible()};

  ImGui::Checkbox("###visible", &state);
  object.setVisible(state);
  ImGui::Separator();

  auto& components = object.components();

  // Attention: do *NOT* modify this for (the iterator postfix increment
  // is necessary to avoid an exception in case the component is removed
  // by the user during the object inspection)
  for (auto end = components.end(), cit = components.begin(); cit != end;)
    inspectComponent(*((cit++)->get()));
}

inline void
SceneWindow::inspectScene()
{
  ImGui::objectNameInput(*_scene);
  ImGui::Separator();
  if (ImGui::CollapsingHeader("Environment"))
  {
    ImGui::colorEdit3("Background", _scene->backgroundColor);
    ImGui::colorEdit3("Ambient Light", _scene->ambientLight);
  }
}

inline void
SceneWindow::inspectCurrentNode()
{
  if (_currentNode == nullptr)
    return;
  if (_currentNode.as<Scene>())
    inspectScene();
  else if (auto sceneObject = _currentNode.as<SceneObject>())
    inspectSceneObject(*sceneObject);
}

void
SceneWindow::inspectorWindow(const char* title)
{
  if (!_showInspector)
    return;
  assert(title != nullptr);
  ImGui::Begin(title);
  inspectCurrentNode();
  ImGui::End();
}

void
SceneWindow::assetsWindow()
{
  if (!_showAssets)
    return;
  ImGui::Begin("Assets");
  if (ImGui::CollapsingHeader("Meshes"))
  {
    auto& meshes = Assets::meshes();

    for (auto mit = meshes.begin(); mit != meshes.end(); ++mit)
    {
      auto name = mit->first.c_str();
      auto selected = false;

      ImGui::Selectable(name, &selected);
      if (ImGui::BeginDragDropSource())
      {
        Assets::loadMesh(mit);
        ImGui::Text(name);
        ImGui::SetDragDropPayload("TriangleMesh", &mit, sizeof(mit));
        ImGui::EndDragDropSource();
      }
    }
  }
  ImGui::Separator();
  if (ImGui::CollapsingHeader("Materials"))
  {
    auto& materials = Assets::materials();

    for (auto mit = materials.begin(); mit != materials.end(); ++mit)
    {
      auto name = mit->first.c_str();
      auto selected = false;

      ImGui::Selectable(name, &selected);
      if (ImGui::BeginDragDropSource())
      {
        ImGui::Text(name);
        ImGui::SetDragDropPayload("Material", &mit, sizeof(mit));
        ImGui::EndDragDropSource();
      }
    }
  }
  ImGui::End();
}

SceneObject*
SceneWindow::pickObject(SceneObject* object, const Ray3f& ray, float& t) const
{
  if (!object->visible())
    return nullptr;

  SceneObject* nearest{};

  for (auto& component : object->components())
    if (auto proxy = dynamic_cast<PrimitiveProxy*>(&*component))
    {
      auto p = proxy->mapper()->primitive();

      if (p->bounds().size().min() == 0)
        puts("Unable to pick scene object");
      else
      {
        Intersection hit;

        if (p->intersect(ray, hit) && hit.distance < t)
        {
          t = hit.distance;
          nearest = object;
        }
      }
      break;
    }
  for (auto& child : object->children())
    if (auto temp = pickObject(&child, ray, t))
      nearest = temp;
  return nearest;
}

SceneObject*
SceneWindow::pickObject(int x, int y) const
{
  auto ray = makeRay(x, y);
  auto t = math::Limits<float>::inf();

  return pickObject(_scene->root(), ray, t);
}

bool
SceneWindow::onPickObject(int x, int y)
{
  if (auto o = pickObject(x, y))
    if (o->selectable())
    {
      if (auto p = _currentNode.as<SceneObject>())
        p->setSelected(false);
      o->setSelected(true);
      _currentNode = o;
    }
  return true;
}

bool
SceneWindow::onPressKey(int key)
{
  if (_viewMode != ViewMode::Editor || key != GLFW_KEY_F)
    return false;
  if (auto object = _currentNode.as<SceneObject>())
  {
    auto editor = this->editor();
    auto camera = editor->camera();
    auto d = camera->viewPlaneNormal();
    auto p = object->transform()->position();

    // TODO: set view angle/height and distance
    camera->setPosition(p + d * editor->orbitDistance());
  }
  return true;
}

} // end namespace graph

} // end namespace cg
