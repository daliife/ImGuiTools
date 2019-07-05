#include <iostream>   
#include <string>
#include "ToolsSystem.h"
#include "extern.h"
#include "Parsers.h"

static bool no_titlebar = false;
static bool no_scrollbar = false;
static bool no_menu = true;
static bool no_move = false;
static bool no_resize = false;
static bool no_collapse = false;
static bool no_close = false;
static bool no_nav = false;
static bool no_background = false;
static bool no_bring_to_front = false;
ImGuiWindowFlags window_flags = 0;

static bool show_hierarchy = true;
static bool show_materials = true;
static bool show_inspector = true;
static bool show_statistics = true;
static bool show_quick_actions = true;
static bool show_app_console = true;

static void UpdateConsole(bool* p_open);

ToolsSystem::~ToolsSystem() { }

void ToolsSystem::init(GraphicsSystem* gs) {
	
	//Styling IMGUI
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = 2.5f;
	style.WindowBorderSize = 0.0f;
	style.FrameBorderSize = 0.0f;
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

	//Variable initialization
	graphics_system_ = gs;
	ent_picked_ray_id_ = -1;
	best = 100.0f;
	worse = 0.0f;
	for (size_t i = 0; i < MAX_VALUES; i++) {
		fpslist[i] = 0.0f;
	}
	
}

void ToolsSystem::lateInit() {

	//picking collider
	ent_picking_ray_ = ECS.createEntity("picking_ray");
	Collider& picking_ray = ECS.createComponentForEntity<Collider>(ent_picking_ray_);
	picking_ray.collider_type = ColliderTypeRay;
	picking_ray.direction = lm::vec3(0, 0, -1);
	picking_ray.max_distance = 0.001f;

}

void ToolsSystem::update(float dt, float current_fps) {

	ImGuiIO &io = ImGui::GetIO();

	if (show_imGUI_) {
		
		fps = current_fps;

		if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
		if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
		if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
		if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
		if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
		if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
		if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
		if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
		if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		//if (no_close)           p_open = NULL; // Don't pass our bool* to Begin

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (io.WantCaptureMouse)
			can_fire_picking_ray_ = false;
		else
			can_fire_picking_ray_ = true;

		if (show_hierarchy) UpdateHierarchy();
		if (show_materials) UpdateMaterials();
		if (show_inspector) UpdateInspector();
		if (show_statistics) UpdateStatistics();
		if (show_quick_actions) UpdateQuickActions();
		if (show_app_console) UpdateConsole(&show_app_console);
		
		UpdateMenuBar(io);
		//ImGui::ShowDemoWindow();

		const float DISTANCE = 20.0f;
		static int corner = 3;
		if (corner != -1) {
			ImVec2 window_pos = ImVec2((corner & 1) ? io.DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? io.DisplaySize.y - DISTANCE : DISTANCE);
			ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		}
		static bool* p_open = nullptr;
		ImGui::SetNextWindowBgAlpha(1.0f); // Transparent background
		if (ImGui::Begin("Instructions", p_open, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
		{
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "DEBUG MODE ACTIVE");
			ImGui::Text("Press 'ALT + 0' to toggle it between On and Off.");
		}
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	}

}

void ToolsSystem::UpdateHierarchy() {

	ImGui::SetNextWindowSize(ImVec2(250, 100), ImGuiCond_FirstUseEver);
	ImGui::Begin("HIERARCHY", &show_hierarchy, window_flags);

	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Text("Shows all the entities of the current scene. ");
	ImGui::SameLine();
	if (!hierarchy_toggle) {
		if (ImGui::SmallButton("Show All")) {
			hierarchy_mode = 1;
			hierarchy_toggle = true;
		}
	}
	else {
		if (ImGui::SmallButton("Hide All")) {
			hierarchy_mode = 2;
			hierarchy_toggle = false;
		}
	}
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Separator();

	Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
	Transform& cam_transform = ECS.getComponentFromEntity<Transform>(cam.owner);
	ImGui::Dummy(ImVec2(0.0f, 5));
	ImGui::SetNextTreeNodeOpen(true);
	if (ImGui::TreeNode("Scene")) {

		//Create an unfoldable tree node called 'Camera'
		if (hierarchy_mode != 0) ImGui::SetNextTreeNodeOpen(hierarchy_mode == 1 ? true : false);
		if (ImGui::TreeNode("Camera")) {
			//create temporary arrays with position and direction data
			float cam_pos_array[3] = { cam.position.x, cam.position.y, cam.position.z };
			float cam_dir_array[3] = { cam.forward.x, cam.forward.y, cam.forward.z };

			//create imGUI components that allow us to change the values when click-dragging
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::DragFloat3("Position", cam_pos_array);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::DragFloat3("Direction", cam_dir_array);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			//use values of temporary arrays to set real values (in case user changes)
			cam.position = lm::vec3(cam_pos_array[0], cam_pos_array[1], cam_pos_array[2]);
			cam_transform.position(cam.position);
			cam.forward = lm::vec3(cam_dir_array[0], cam_dir_array[1], cam_dir_array[2]).normalize();
			ImGui::TreePop();
		}

		// 1) create a temporary array with ALL transforms
		std::vector<TransformNode> transform_nodes;
		auto& all_transforms = ECS.getAllComponents<Transform>();
		for (size_t i = 0; i < all_transforms.size(); i++) {
			TransformNode tn;
			tn.trans_id = (int)i;
			tn.entity_owner = all_transforms[i].owner;
			tn.ent_name = ECS.entities[tn.entity_owner].name;
			if (all_transforms[i].parent == -1)
				tn.isTop = true;
			transform_nodes.push_back(tn);
		}

		// 2) traverse array to assign children to their parents
		for (size_t i = 0; i < transform_nodes.size(); i++) {
			int parent = all_transforms[i].parent;
			if (parent != -1) {
				transform_nodes[parent].children.push_back(transform_nodes[i]);
			}
		}

		// 3) create a new array with only top level nodes of transform tree
		std::vector<TransformNode> transform_topnodes;
		for (size_t i = 0; i < transform_nodes.size(); i++) {
			if (transform_nodes[i].isTop)
				transform_topnodes.push_back(transform_nodes[i]);
		}

		//draw all the nodes
		for (auto& trans : transform_topnodes) {
			imGuiRenderTransformNode(trans);
		}

		ImGui::TreePop();

	}

	ImGui::End();

}

void ToolsSystem::UpdateMaterials() {

	ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowBgAlpha(1.0);
	ImGui::Begin("MATERIALS", &show_materials, window_flags);

	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Text("List of all material maps in the scene.");
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	if (!material_toggle) {
		if (ImGui::SmallButton("Show All")) {
			material_mode = 1;
			material_toggle = true;
		}
	}
	else {
		if (ImGui::SmallButton("Hide All")) {
			material_mode = 2;
			material_toggle = false;
		}
	}
	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	std::vector<Material>& mat = graphics_system_->getMaterials();
	for (size_t i = 0; i < mat.size(); i++) {
		if (material_mode != 0) ImGui::SetNextTreeNodeOpen(material_mode == 1 ? true : false);
		if (ImGui::TreeNode(mat[i].name.c_str())) {

			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			if (mat[i].diffuse_map != -1) {
				ImGui::Image((ImTextureID)(mat[i].diffuse_map), ImVec2(64, 64));
				ImGui::SameLine();
				ImGui::SetCursorPos({ ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + (64 - ImGui::GetFont()->FontSize) / 2 });
				ImGui::Text(" Diffuse Material Map");
			}
			if (mat[i].normal_map != -1) {
				ImGui::Image((ImTextureID)(mat[i].normal_map), ImVec2(64, 64));
				ImGui::SameLine();
				ImGui::SetCursorPos({ ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + (64 - ImGui::GetFont()->FontSize) / 2 });
				ImGui::Text(" Normal Material Map");
			}
			if (mat[i].specular_map != -1) {
				ImGui::Image((ImTextureID)(mat[i].specular_map), ImVec2(64, 64));
				ImGui::SameLine();
				ImGui::SetCursorPos({ ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + (64 - ImGui::GetFont()->FontSize) / 2 });
				ImGui::Text(" Specular Material Map");
			}

			if (mat[i].diffuse_map == -1 && mat[i].normal_map == -1 && mat[i].specular_map == -1) {
				ImGui::Dummy(ImVec2(0.0f, 15.0f));
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "NO TEXTURE MAPS");
				ImGui::Dummy(ImVec2(0.0f, 15.0f));
			}

			ImGui::TreePop();
		}

	}

	if (material_mode != 0) material_mode = 0;

	ImGui::End();
}

void ToolsSystem::UpdateInspector() {

	ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_FirstUseEver);
	ImGui::Begin("INSPECTOR", &show_inspector, window_flags);

	Collider& pick_ray_collider = ECS.getComponentFromEntity<Collider>(ent_picking_ray_);

	if (pick_ray_collider.colliding /*|| ent_picked_ray_id_ != -1*/) {

		Collider& picked_collider = ECS.getComponentInArray<Collider>(pick_ray_collider.other);
		ent_picked_ray_id_ = picked_collider.owner;
		Transform& picked_transform = ECS.getComponentFromEntity<Transform>(ent_picked_ray_id_);

		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		ImGui::Text("SELECTED ENTITY: ");	
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), ECS.entities[ent_picked_ray_id_].name.c_str());
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		ImGui::Separator();

		lm::vec3 pos = picked_transform.position();
		lm::vec3 front = picked_transform.front();
		float pos_array[3] = { pos.x, pos.y, pos.z };
		float rot_array[3] = { picked_transform.m[8], picked_transform.m[9],picked_transform.m[10] };
		float scal_array[3] = { picked_transform.m[0], picked_transform.m[5], picked_transform.m[10] };

		ImGui::Dummy(ImVec2(0.0f, 5.0f));

		if (!inspector_toggle) {
			if (ImGui::Button("Show All")) {
				inspector_mode = 1;
				inspector_toggle = true;
			}
		} else {
			if (ImGui::Button("Hide All")) {
				inspector_mode = 2;
				inspector_toggle = false;
			}
		}

		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		if (inspector_mode != 0) ImGui::SetNextTreeNodeOpen(inspector_mode == 1 ? true : false);
		if (ImGui::TreeNode("Transform")) {
			
			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			if (ImGui::DragFloat3("Position", pos_array)) {
				picked_transform.position(pos_array[0], pos_array[1], pos_array[2]);
			}

			if (ImGui::DragFloat3("Rotation", rot_array)) {
				picked_transform.m[8] = rot_array[0];
				picked_transform.m[9] = rot_array[1];
				picked_transform.m[10] = rot_array[2];
			}

			if (ImGui::DragFloat3("Scale", scal_array)) {
				picked_transform.m[0] = scal_array[0];
				picked_transform.m[5] = scal_array[1];
				picked_transform.m[10] = scal_array[2];
			}

			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			ImGui::TreePop();

		}

		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		if (inspector_mode != 0) ImGui::SetNextTreeNodeOpen(inspector_mode == 1 ? true : false);
		if (ImGui::TreeNode("Render")) {
			
			Material& mat = graphics_system_->getMaterial(ent_picked_ray_id_-1);

			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			if (mat.diffuse_map != -1) {
				ImGui::Image((ImTextureID)(mat.diffuse_map), ImVec2(64, 64));
				ImGui::SameLine();
				ImGui::SetCursorPos({ ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + (64 - ImGui::GetFont()->FontSize) / 2 });
				ImGui::Text(" Diffuse Material Map");
			}
			if (mat.normal_map != -1) {
				ImGui::Image((ImTextureID)(mat.normal_map), ImVec2(64, 64));
				ImGui::SameLine();
				ImGui::SetCursorPos({ ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + (64 - ImGui::GetFont()->FontSize) / 2 });
				ImGui::Text(" Normal Material Map");
			}
			if (mat.specular_map != -1) {
				ImGui::Image((ImTextureID)(mat.specular_map), ImVec2(64, 64));
				ImGui::SameLine();
				ImGui::SetCursorPos({ ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + (64 - ImGui::GetFont()->FontSize) / 2 });
				ImGui::Text(" Specular Material Map");
			}

			if (mat.diffuse_map == -1 && mat.normal_map == -1 && mat.specular_map == -1) {
				ImGui::Dummy(ImVec2(0.0f, 15.0f));
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "NO TEXTURE MAPS");
				ImGui::Dummy(ImVec2(0.0f, 15.0f));
			}

			float ambient[3] = { mat.ambient.r, mat.ambient.g, mat.ambient.b };
			ImGui::ColorEdit3("Ambient", ambient);
			mat.ambient = lm::vec3(ambient[0], ambient[1], ambient[2]);

			float diffuse[3] = { mat.diffuse.r, mat.diffuse.g, mat.diffuse.b };
			ImGui::ColorEdit3("Diffuse", diffuse);
			mat.diffuse = lm::vec3(diffuse[0], diffuse[1], diffuse[2]);

			float specular[3] = { mat.specular.r, mat.specular.g, mat.specular.b };
			ImGui::ColorEdit3("Specular", specular);
			mat.specular = lm::vec3(specular[0], specular[1], specular[2]);

			float specular_gloss = mat.specular_gloss;
			ImGui::DragFloat("Glossyness", &specular_gloss, 0.01f, 0, 1);
			mat.specular_gloss = specular_gloss;

			float normal_factor = mat.normal_factor;
			ImGui::DragFloat("Normal factor", &normal_factor, 0.01f, 0, 2);
			mat.normal_factor = normal_factor;

			float height = mat.height;
			ImGui::DragFloat("Normal height", &height, 0.01f, 0, 2);
			mat.height = height;
				
			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			ImGui::TreePop();

		}

		ImGui::Dummy(ImVec2(0.0f, 5.0f));

		if (inspector_mode != 0) inspector_mode = 0;

	} else {

		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		ImGui::Text("SELECTED ENTITY: ");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "nothing selected");
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		ImGui::TextDisabled("Select any object with your mouse to show its details here.");
	}

	ImGui::End();

}

void ToolsSystem::UpdateStatistics() {
	
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
	ImGui::Begin("STATISTICS", &show_statistics, window_flags);

	static int values_offset = 0;
	static double refresh_time = 0.0;
	if (refresh_time == 0.0)
		refresh_time = ImGui::GetTime();
	while (refresh_time < ImGui::GetTime()) {
		for (size_t i = 1; i < MAX_VALUES; i++) {
			fpslist[i - 1] = fpslist[i];
		}
		fpslist[MAX_VALUES-1] = fps;
		values_offset = (values_offset + 1) % IM_ARRAYSIZE(fpslist);
		refresh_time += 1.0f;
	}
	ImGui::PlotLines(("Framerate \n" + to_string(fps) + " ms").c_str(), fpslist, IM_ARRAYSIZE(fpslist), 0, NULL, 0.0f, worse, ImVec2(0, 100));

	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	if (fps < best && fps > 0) best = fps;
	if (fps > worse && fps > best) worse = fps;
	
	ImGui::Text("Framerate: ");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(1, 1, 0, 1), (to_string(fps) + " ms").c_str());
	ImGui::Text("Best: ");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 1, 0, 1), (to_string(best) + " ms").c_str());
	ImGui::Text("Worse: ");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(1, 0, 0, 1), (to_string(worse) + " ms").c_str());

	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	if (ImGui::Button("Reset values")) {
		best = 100.0f;
		worse = 0.0f;
		for (size_t i = 0; i < MAX_VALUES; i++) {
			fpslist[i] = 0.0f;
		}
	}

	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::End();

}

void ToolsSystem::UpdateQuickActions() {
	
	ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
	ImGui::Begin("QUICK ACTIONS", &show_quick_actions, window_flags);
	
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Text("Function/Debug checks");
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	ImGui::Checkbox("Debug lines", &debugState); ImGui::SameLine(150);
	ImGui::Checkbox("Show skybox", &environmentState); ImGui::SameLine(300);
	static bool showColors = false;
	ImGui::Checkbox("Background Color", &showColors); ImGui::SameLine(150);
	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	static float background_color[4] = { 0,0,0,1 };
	if (showColors) {
		ImGui::ColorEdit4("Selected color", background_color);
		graphics_system_->setBackgroundColor(background_color);
	}

	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Text("View selector");
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
	Transform& cam_transform = ECS.getComponentFromEntity<Transform>(cam.owner);
	lm::mat4 R_yaw, R_pitch;

	//lm::vec3 v1 = cam.forward;
	//lm::vec3 v2 = cam.up;
	//float theta = std::acos((v1.dot(v2)) / (v1.distance * v2.distance));
	//printf(to_string(theta).c_str());

	ImVec2 button_sz(100, 30);
	if (ImGui::Button("Top", button_sz)) {
		R_yaw.makeRotationMatrix(DEG2RAD *90, lm::vec3(0, 1, 0));
		cam.forward = R_yaw * cam.forward;
		lm::vec3 pitch_axis = cam.forward.normalize().cross(lm::vec3(0, 1, 0));
		R_pitch.makeRotationMatrix(1, pitch_axis);
		cam.forward = R_pitch * cam.forward;
	}
	ImGui::SameLine(150);
	if (ImGui::Button("Bottom", button_sz)) {
		R_yaw.makeRotationMatrix(1, lm::vec3(0, 1, 0));
		cam.forward = R_yaw * cam.forward;
		lm::vec3 pitch_axis = cam.forward.normalize().cross(lm::vec3(0, -1, 0));
		R_pitch.makeRotationMatrix(1, pitch_axis);
		cam.forward = R_pitch * cam.forward;
	}
	ImGui::SameLine(300);
	if (ImGui::Button("Lateral", button_sz)) {
		R_yaw.makeRotationMatrix(1, lm::vec3(0, 1, 0));
		cam.forward = R_yaw * cam.forward;
	}

	

	ImGui::End();
}

void ToolsSystem::UpdateMenuBar(ImGuiIO &io)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Window"))
		{
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Checkbox("Hierarchy", &show_hierarchy);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Checkbox("Materials", &show_materials);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Checkbox("Inspector", &show_inspector);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Checkbox("Statistics", &show_statistics);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Checkbox("Quick actions", &show_quick_actions);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Checkbox("Console", &show_app_console);
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			if(ImGui::SmallButton("Toggle All")) {
				show_hierarchy = !show_hierarchy;
				show_materials = !show_materials;
				show_inspector = !show_inspector;
				show_statistics = !show_statistics;
				show_quick_actions = !show_quick_actions;
				show_app_console = !show_app_console;
			};
			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			ImGui::EndMenu();
		}
		ImGui::SameLine(io.DisplaySize.x - 180);
		string temp = (to_string(fps) + " ms");
		const char * test = temp.c_str();
		ImGui::Text("Framerate:");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), test);

		ImGui::EndMainMenuBar();
	}
}

struct PersonalisedConsole
{
	char                  InputBuf[256];
	ImVector<char*>       Items;
	ImVector<const char*> Commands;
	ImVector<char*>       History;
	int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
	ImGuiTextFilter       Filter;
	bool                  AutoScroll;
	bool                  ScrollToBottom;

	PersonalisedConsole()
	{
		ClearLog();
		memset(InputBuf, 0, sizeof(InputBuf));
		HistoryPos = -1;
		Commands.push_back("HELP");
		Commands.push_back("HISTORY");
		Commands.push_back("CLEAR");
		Commands.push_back("CLASSIFY");  // "classify" is only here to provide an example of "C"+[tab] completing to "CL" and displaying matches.
		AutoScroll = true;
		ScrollToBottom = true;
		AddLog("Type some command and press ENTER to execute it.");
	}
	~PersonalisedConsole()
	{
		ClearLog();
		for (int i = 0; i < History.Size; i++)
			free(History[i]);
	}

	// Portable helpers
	static int   Stricmp(const char* str1, const char* str2) { int d; while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; } return d; }
	static int   Strnicmp(const char* str1, const char* str2, int n) { int d = 0; while (n > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; n--; } return d; }
	static char* Strdup(const char *str) { size_t len = strlen(str) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)str, len); }
	static void  Strtrim(char* str) { char* str_end = str + strlen(str); while (str_end > str && str_end[-1] == ' ') str_end--; *str_end = 0; }

	void    ClearLog()
	{
		for (int i = 0; i < Items.Size; i++)
			free(Items[i]);
		Items.clear();
		ScrollToBottom = true;
	}

	void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
	{
		// FIXME-OPT
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
		buf[IM_ARRAYSIZE(buf) - 1] = 0;
		va_end(args);
		Items.push_back(Strdup(buf));
		if (AutoScroll)
			ScrollToBottom = true;
	}

	void    Draw(const char* title, bool* p_open)
	{
		ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin(title, p_open))
		{
			ImGui::End();
			return;
		}

		// As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar. So e.g. IsItemHovered() will return true when hovering the title bar.
		// Here we create a context menu only available from the title bar.
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Close Console"))
				*p_open = false;
			ImGui::EndPopup();
		}

		ImGui::TextWrapped("Enter 'HELP' for help, press TAB to use text completion.");

		// TODO: display items starting from the bottom

		//if (ImGui::SmallButton("Add Dummy Text")) { AddLog("%d some text", Items.Size); AddLog("some more text"); AddLog("display very important message here!"); } ImGui::SameLine();
		//if (ImGui::SmallButton("Add Dummy Error")) { AddLog("[error] something went wrong"); } ImGui::SameLine();
		if (ImGui::SmallButton("Clear")) { ClearLog(); } ImGui::SameLine();
		bool copy_to_clipboard = ImGui::SmallButton("Copy"); ImGui::SameLine();
		if (ImGui::SmallButton("Scroll to bottom")) ScrollToBottom = true;
		//static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

		ImGui::Separator();

		// Options menu
		if (ImGui::BeginPopup("Options"))
		{
			if (ImGui::Checkbox("Auto-scroll", &AutoScroll))
				if (AutoScroll)
					ScrollToBottom = true;
			ImGui::EndPopup();
		}

		// Options, Filter
		if (ImGui::Button("Options"))
			ImGui::OpenPopup("Options");
		ImGui::SameLine();
		Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
		ImGui::Separator();

		const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear")) ClearLog();
			ImGui::EndPopup();
		}

		// Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
		// NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping to only process visible items.
		// You can seek and display only the lines that are visible using the ImGuiListClipper helper, if your elements are evenly spaced and you have cheap random access to the elements.
		// To use the clipper we could replace the 'for (int i = 0; i < Items.Size; i++)' loop with:
		//     ImGuiListClipper clipper(Items.Size);
		//     while (clipper.Step())
		//         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		// However, note that you can not use this code as is if a filter is active because it breaks the 'cheap random-access' property. We would need random-access on the post-filtered list.
		// A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices that passed the filtering test, recomputing this array when user changes the filter,
		// and appending newly elements as they are inserted. This is left as a task to the user until we can manage to improve this example code!
		// If your items are of variable size you may want to implement code similar to what ImGuiListClipper does. Or split your data into fixed height items to allow random-seeking into your list.
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		if (copy_to_clipboard)
			ImGui::LogToClipboard();
		for (int i = 0; i < Items.Size; i++)
		{
			const char* item = Items[i];
			if (!Filter.PassFilter(item))
				continue;

			// Normally you would store more information in your item (e.g. make Items[] an array of structure, store color/type etc.)
			bool pop_color = false;
			if (strstr(item, "[error]")) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f)); pop_color = true; }
			else if (strncmp(item, "# ", 2) == 0) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f)); pop_color = true; }
			ImGui::TextUnformatted(item);
			if (pop_color)
				ImGui::PopStyleColor();
		}
		if (copy_to_clipboard)
			ImGui::LogFinish();
		if (ScrollToBottom)
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;
		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::Separator();

		// Command-line
		bool reclaim_focus = false;
		if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
		{
			char* s = InputBuf;
			Strtrim(s);
			if (s[0])
				ExecCommand(s);
			strcpy(s, "");
			reclaim_focus = true;
		}

		// Auto-focus on window apparition
		ImGui::SetItemDefaultFocus();
		if (reclaim_focus)
			ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

		ImGui::End();
	}

	void    ExecCommand(const char* command_line)
	{
		AddLog("# %s\n", command_line);

		// Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
		HistoryPos = -1;
		for (int i = History.Size - 1; i >= 0; i--)
			if (Stricmp(History[i], command_line) == 0)
			{
				free(History[i]);
				History.erase(History.begin() + i);
				break;
			}
		History.push_back(Strdup(command_line));

		// Process command
		if (Stricmp(command_line, "CLEAR") == 0)
		{
			ClearLog();
		}
		else if (Stricmp(command_line, "HELP") == 0)
		{
			AddLog("Commands:");
			for (int i = 0; i < Commands.Size; i++)
				AddLog("- %s", Commands[i]);
		}
		else if (Stricmp(command_line, "HISTORY") == 0)
		{
			int first = History.Size - 10;
			for (int i = first > 0 ? first : 0; i < History.Size; i++)
				AddLog("%3d: %s\n", i, History[i]);
		}
		else if (Stricmp(command_line, "ADDSPHERE") == 0)
		{
			int sphere_entity = ECS.createEntity("test_sphere");
			ECS.getComponentFromEntity<Transform>(sphere_entity).translate(5.0f, 4.0f, 2.0f);
			Mesh& sphere_mesh = ECS.createComponentForEntity<Mesh>(sphere_entity);
			sphere_mesh.geometry = 2;
			sphere_mesh.material = 1;
			Collider& sphere_collider = ECS.createComponentForEntity<Collider>(sphere_entity);
			sphere_collider.collider_type = ColliderTypeBox;
			sphere_collider.local_halfwidth = lm::vec3(1, 1, 1);
			sphere_collider.max_distance = 100.0f;

		}
		else
		{
			AddLog("Unknown command: '%s'\n", command_line);
		}
		
		// On commad input, we scroll to bottom even if AutoScroll==false
		ScrollToBottom = true;
	}

	static int TextEditCallbackStub(ImGuiInputTextCallbackData* data) // In C++11 you are better off using lambdas for this sort of forwarding callbacks
	{
		PersonalisedConsole* console = (PersonalisedConsole*)data->UserData;
		return console->TextEditCallback(data);
	}

	int     TextEditCallback(ImGuiInputTextCallbackData* data)
	{
		//AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
		switch (data->EventFlag)
		{
		case ImGuiInputTextFlags_CallbackCompletion:
		{
			// Example of TEXT COMPLETION

			// Locate beginning of current word
			const char* word_end = data->Buf + data->CursorPos;
			const char* word_start = word_end;
			while (word_start > data->Buf)
			{
				const char c = word_start[-1];
				if (c == ' ' || c == '\t' || c == ',' || c == ';')
					break;
				word_start--;
			}

			// Build a list of candidates
			ImVector<const char*> candidates;
			for (int i = 0; i < Commands.Size; i++)
				if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
					candidates.push_back(Commands[i]);

			if (candidates.Size == 0)
			{
				// No match
				AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
			}
			else if (candidates.Size == 1)
			{
				// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0]);
				data->InsertChars(data->CursorPos, " ");
			}
			else
			{
				// Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display "CLEAR" and "CLASSIFY"
				int match_len = (int)(word_end - word_start);
				for (;;)
				{
					int c = 0;
					bool all_candidates_matches = true;
					for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
						if (i == 0)
							c = toupper(candidates[i][match_len]);
						else if (c == 0 || c != toupper(candidates[i][match_len]))
							all_candidates_matches = false;
					if (!all_candidates_matches)
						break;
					match_len++;
				}

				if (match_len > 0)
				{
					data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
					data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
				}

				// List matches
				AddLog("Possible matches:\n");
				for (int i = 0; i < candidates.Size; i++)
					AddLog("- %s\n", candidates[i]);
			}

			break;
		}
		case ImGuiInputTextFlags_CallbackHistory:
		{
			// Example of HISTORY
			const int prev_history_pos = HistoryPos;
			if (data->EventKey == ImGuiKey_UpArrow)
			{
				if (HistoryPos == -1)
					HistoryPos = History.Size - 1;
				else if (HistoryPos > 0)
					HistoryPos--;
			}
			else if (data->EventKey == ImGuiKey_DownArrow)
			{
				if (HistoryPos != -1)
					if (++HistoryPos >= History.Size)
						HistoryPos = -1;
			}

			// A better implementation would preserve the data on the current input line along with cursor position.
			if (prev_history_pos != HistoryPos)
			{
				const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, history_str);
			}
		}
		}
		return 0;
	}
};

struct ExampleAppLog
{
	ImGuiTextBuffer     Buf;
	ImGuiTextFilter     Filter;
	ImVector<int>       LineOffsets;        // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
	bool                AutoScroll;
	bool                ScrollToBottom;

	ExampleAppLog()
	{
		AutoScroll = true;
		ScrollToBottom = false;
		Clear();
	}

	void    Clear()
	{
		Buf.clear();
		LineOffsets.clear();
		LineOffsets.push_back(0);
	}

	void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
	{
		int old_size = Buf.size();
		va_list args;
		va_start(args, fmt);
		Buf.appendfv(fmt, args);
		va_end(args);
		for (int new_size = Buf.size(); old_size < new_size; old_size++)
			if (Buf[old_size] == '\n')
				LineOffsets.push_back(old_size + 1);
		if (AutoScroll)
			ScrollToBottom = true;
	}

	void    Draw(const char* title, bool* p_open = NULL)
	{
		if (!ImGui::Begin(title, p_open))
		{
			ImGui::End();
			return;
		}

		// Options menu
		if (ImGui::BeginPopup("Options"))
		{
			if (ImGui::Checkbox("Auto-scroll", &AutoScroll))
				if (AutoScroll)
					ScrollToBottom = true;
			ImGui::EndPopup();
		}

		// Main window
		if (ImGui::Button("Options"))
			ImGui::OpenPopup("Options");
		ImGui::SameLine();
		bool clear = ImGui::Button("Clear");
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::SameLine();
		Filter.Draw("Filter", -100.0f);

		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		if (clear)
			Clear();
		if (copy)
			ImGui::LogToClipboard();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		const char* buf = Buf.begin();
		const char* buf_end = Buf.end();
		if (Filter.IsActive())
		{
			// In this example we don't use the clipper when Filter is enabled.
			// This is because we don't have a random access on the result on our filter.
			// A real application processing logs with ten of thousands of entries may want to store the result of search/filter.
			// especially if the filtering function is not trivial (e.g. reg-exp).
			for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
			{
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
				if (Filter.PassFilter(line_start, line_end))
					ImGui::TextUnformatted(line_start, line_end);
			}
		}
		else
		{
			// The simplest and easy way to display the entire buffer:
			//   ImGui::TextUnformatted(buf_begin, buf_end);
			// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward to skip non-visible lines.
			// Here we instead demonstrate using the clipper to only process lines that are within the visible area.
			// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them on your side is recommended.
			// Using ImGuiListClipper requires A) random access into your data, and B) items all being the  same height,
			// both of which we can handle since we an array pointing to the beginning of each line of text.
			// When using the filter (in the block of code above) we don't have random access into the data to display anymore, which is why we don't use the clipper.
			// Storing or skimming through the search result would make it possible (and would be recommended if you want to search through tens of thousands of entries)
			ImGuiListClipper clipper;
			clipper.Begin(LineOffsets.Size);
			while (clipper.Step())
			{
				for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
				{
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
					ImGui::TextUnformatted(line_start, line_end);
				}
			}
			clipper.End();
		}
		ImGui::PopStyleVar();

		if (ScrollToBottom)
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;
		ImGui::EndChild();
		ImGui::End();
	}
};

static void UpdateConsole(bool* p_open) {

	static PersonalisedConsole console;
	console.Draw("CONSOLE", p_open);

}

void ToolsSystem::setPickingRay(int mouse_x, int mouse_y, int screen_width, int screen_height) {
	
	//if we are not in debug mode (alt-0) do nothing!
	if (!can_fire_picking_ray_) return;

	//convert mouse_x and mouse_y to NDC
	float ndc_x = (((float)mouse_x / (float)screen_width) * 2) - 1;
	float ndc_y = (((float)(screen_height - mouse_y) / (float)screen_height) * 2) - 1;

	//start point for ray is point on near plane of ndc
	lm::vec4 mouse_near_plane(ndc_x, ndc_y, -1.0, 1.0);

	//get view projection
	Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
	lm::mat4 inv_vp = cam.view_projection;
	inv_vp.inverse();

	//get ray start point in world coordinates
	//don't forget this is in HOMOGENOUS coords :)
	lm::vec4 mouse_world = inv_vp * mouse_near_plane;
	//so we must normalize the vector
	mouse_world.normalize();
	lm::vec3 mouse_world_3(mouse_world.x, mouse_world.y, mouse_world.z);

	//set the picking ray
	//the actual collision detection will be done next frame in the CollisionSystem
	Transform& pick_ray_transform = ECS.getComponentFromEntity<Transform>(ent_picking_ray_);
	Collider& pick_ray_collider = ECS.getComponentFromEntity<Collider>(ent_picking_ray_);
	pick_ray_transform.position(cam.position);
	pick_ray_collider.direction = (mouse_world_3 - cam.position).normalize();
	pick_ray_collider.max_distance = 1000000;

	////Set id picked ray object
	//if (pick_ray_collider.other != -1) {
	//	Collider& picked_collider = ECS.getComponentInArray<Collider>(pick_ray_collider.other);
	//	ent_picked_ray_id_ = picked_collider.owner;
	//} else {
	//	ent_picked_ray_id_ != -1;
	//}

	//print(to_string(ent_picked_ray_id_).c_str());

}

void ToolsSystem::imGuiRenderTransformNode(TransformNode& trans) {
	auto& ent = ECS.entities[trans.entity_owner];
	if (hierarchy_mode != 0) ImGui::SetNextTreeNodeOpen(hierarchy_mode == 1 ? true : false);
	if (ImGui::TreeNode(ent.name.c_str())) {
		Transform& transform = ECS.getComponentFromEntity<Transform>(ent.name);
		if (ECS.getComponentID<Light>(trans.entity_owner) != -1) {
			graphics_system_->needUpdateLights = true;
		}
		lm::vec3 pos = transform.position();
		float pos_array[3] = { pos.x, pos.y, pos.z };
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		ImGui::DragFloat3("Position", pos_array);
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		transform.position(pos_array[0], pos_array[1], pos_array[2]);

		for (auto& child : trans.children) {
			imGuiRenderTransformNode(child);
		}
		ImGui::TreePop();
	}
	if (inspector_mode != 0) inspector_mode = 0;
}
