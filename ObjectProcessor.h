#pragma once
#include <vector>
#include <memory>

struct ObjectData;

class Camera;
class InputManager;

class ObjectProcessor {
public:
	ObjectProcessor(std::shared_ptr<InputManager> pInputManager);
	void SetCamera(std::shared_ptr<Camera> const& pCamera);
	ObjectData& AddStaticMesh(ObjectData transform);
	~ObjectProcessor();

	void ProcessObjects(float deltaTime);
private:
	std::shared_ptr<Camera> m_pCamera;
	std::vector<ObjectData> m_meshTransforms;
	std::shared_ptr<InputManager> m_pInputManager;
};

using ObjectProcessorPtr_t = std::shared_ptr<ObjectProcessor>;