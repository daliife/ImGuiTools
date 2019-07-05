#pragma once
#include "includes.h"
#include "Shader.h"
#include <vector>
#include "GraphicsSystem.h"

struct TransformNode {
	std::vector<TransformNode> children;
	int trans_id;
	int entity_owner;
	std::string ent_name;
	bool isTop = false;
};

static const int MAX_VALUES = 20;

class ToolsSystem {
public:

	~ToolsSystem();
	void init(GraphicsSystem* gs);
	void lateInit();
	void update(float dt, float current_fps);

	bool isShowGUI() { return show_imGUI_; };
	void toggleimGUI() { show_imGUI_ = !show_imGUI_; };
	void setPickingRay(int mouse_x, int mouse_y, int screen_width, int screen_height);
	bool getDebugState() { return debugState; };
	bool getEnvironmentState() { return environmentState; };

private:
	
	float fpslist[MAX_VALUES];
	GraphicsSystem* graphics_system_;

	void imGuiRenderTransformNode(TransformNode& trans);
	bool show_imGUI_ = true;
	bool debugState = true;
	bool environmentState = true;

	void UpdateHierarchy();
	void UpdateMaterials();
	void UpdateInspector();
	void UpdateStatistics();
	void UpdateQuickActions();
	void UpdateMenuBar(ImGuiIO &io);

	bool can_fire_picking_ray_ = true;
	int ent_picking_ray_;
	int ent_picked_ray_id_;

	int hierarchy_mode = 0;
	bool hierarchy_toggle = false;
	int inspector_mode = 0;
	bool inspector_toggle = false;
	int material_mode = 0;
	bool material_toggle = false;

	float fps = 0.0f;
	float best = 10.0f;
	float worse = 0.0f;
	
};