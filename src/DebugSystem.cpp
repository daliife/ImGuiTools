#include "DebugSystem.h"
#include "extern.h"
#include "Parsers.h"
#include "shaders_default.h"

DebugSystem::~DebugSystem() {
	delete grid_shader_;
	delete icon_shader_;
}

void DebugSystem::init(GraphicsSystem* gs) {
	graphics_system_ = gs;
}

void DebugSystem::lateInit() {
	
	//init booleans
	draw_grid_ = false;
	draw_icons_ = false;
	draw_frustra_ = false;
	draw_colliders_ = false;

	//compile debug shaders from strings in header file
	grid_shader_ = new Shader();
	grid_shader_->compileFromStrings(g_shader_line_vertex, g_shader_line_fragment);
	icon_shader_ = new Shader();
	icon_shader_->compileFromStrings(g_shader_icon_vertex, g_shader_icon_fragment);

	//create geometries
	createGrid_();
	createIcon_();
	createCube_();
	createRay_();

	//create texture for light icon
	icon_light_texture_ = Parsers::parseTexture("data/assets/icon_light.tga");
	icon_camera_texture_ = Parsers::parseTexture("data/assets/icon_camera.tga");

    //bones
    joint_shader_ = new Shader("data/shaders/joints.vert", "data/shaders/joints.frag");
    createJointGeometry_();
    
	setActive(true);
}

//draws debug information or not
void DebugSystem::setActive(bool a) {
    active_ = a;
	draw_grid_ = a;
	draw_icons_ = a;
	draw_frustra_ = a;
	draw_colliders_ = a;
    draw_joints_ = a;
}

bool DebugSystem::isActive() {
	return active_;
}

//called once per frame
void DebugSystem::update(float dt) {

    if (!active_) return;
    
    //line drawing first, use same shader
    if (draw_grid_ || draw_frustra_ || draw_colliders_) {
        
        //use line shader to draw all lines and boxes
        glUseProgram(grid_shader_->program);
        
        if (draw_grid_) {
            drawGrid_();
        }
        
        if (draw_frustra_) {
            drawFrusta_();
        }
        
        if (draw_colliders_) {
            drawColliders_();
        }
    }
    
    //icon drawing
    if (draw_icons_) {
        drawIcons_();
    }
    //joint drawing
    if (draw_joints_) {
        drawJoints_();
    }
       
    glBindVertexArray(0);
    
}

//Recursive function that creates joint index buffer which parent-child indices
void createJointIndexBuffer(Joint* current, std::vector<GLuint>& indices) {
    
    GLuint current_joint_index = current->index_in_chain;
    
    //only draw line if we have a parent
    if (current->parent) {
        //draw line from parent to current
        GLuint parent_index = current->parent->index_in_chain;
        indices.push_back(parent_index);
        indices.push_back(current_joint_index);
    }
    
    for (auto child : current->children){
        createJointIndexBuffer(child, indices);
    }
}

//create joint geometry
//the class member variables joints_vaos_ stores the VAO index for each
//joint chain. So we loop chains and create and array of vertices for
//each joint.
//EACH VERTEX POSITION IS (0,0,0). Why? Because we will pass joint positions
//to shader as uniforms
//However we do have to make an index buffer to draw lines, based on index of
//joints in tree
void DebugSystem::createJointGeometry_() {
    auto& skinnedmeshes = ECS.getAllComponents<SkinnedMesh>();
    for (auto& sm : skinnedmeshes) {
        if (!sm.root) continue; //if this mesh does not have a joint
        
        //count all joints in
        GLuint current_chain_count = sm.num_joints;
        std::vector<float> positions(current_chain_count * 3, 0);
        
        std::vector<GLuint> indices;
        createJointIndexBuffer(sm.root, indices);
        
        GLuint new_vao;
        glGenVertexArrays(1, &new_vao);
        glBindVertexArray(new_vao);
        //positions
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), &(positions[0]), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        //indices
        GLuint ibo;
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &(indices[0]), GL_STATIC_DRAW);
        
        //add to member variable storage
        joints_vaos_.push_back(new_vao);
    }
    
    
    //unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
}

//Recursive function which traverses joint tree depth first
//multiplies each joint model matrix by that of it's parent
//(to create a 'world' matrix for each joint), then copies
//that world matrix to an array of floats (all_matrices) which
//stores all of the joint matrices, one after another
//this float array is passed to the shader as a uniform
// Params:
//--------
// - current: the current joint, should pass root joint at first call
// - current_model: the current global model matrix (pass identity at first call
// - all_matrices: a vector which MUST BE of size 16 * num_joints
// - joint_count: integer passed by reference, used to track where we are in joint chain
void DebugSystem::getJointWorldMatrices_(Joint* current,
                                         lm::mat4 current_model,
                                         std::vector<float>& all_matrices,
                                         int& joint_count) {
    
    lm::mat4 joint_global_model = current_model * current->matrix;
    
    for (int i = 0; i < 16; i++)
        all_matrices[joint_count * 16 + i] = joint_global_model.m[i];
    
    for (auto& c : current->children) {
        joint_count++;
        getJointWorldMatrices_(c, joint_global_model, all_matrices, joint_count);
    }
}

//function that draws joints to screen
void DebugSystem::drawJoints_() {
    
    //joint shader
    glUseProgram(joint_shader_->program);
    
    Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);
    auto& skinnedmeshes = ECS.getAllComponents<SkinnedMesh>();
    
    //skinned_meshes size must be same as joints_vaos size
    for (size_t i = 0; i < skinnedmeshes.size(); i++) {
        if (!skinnedmeshes[i].root) continue; //only draw if has joint chain
        
        //find uniform location (it is an array
        GLint u_model = glGetUniformLocation(joint_shader_->program, "u_model");
        
        //create float vector and fill it with mvps for each joint
        std::vector<float> all_matrices(skinnedmeshes[i].num_joints * 16, 0);
        int joint_counter = 0;
        getJointWorldMatrices_(skinnedmeshes[i].root, lm::mat4(), all_matrices, joint_counter);
        
        //send to shader
        glUniformMatrix4fv(u_model, skinnedmeshes[i].num_joints, GL_FALSE, &all_matrices[0]);
        
        joint_shader_->setUniform(U_VP, cam.view_projection);
        
        glBindVertexArray(joints_vaos_[i]);
        glDrawElements(GL_LINES, skinnedmeshes[i].num_joints * 2 , GL_UNSIGNED_INT, 0);
    }
}

void DebugSystem::drawGrid_() {
    //get the camera view projection matrix
    lm::mat4 vp = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
    
    //use line shader to draw all lines and boxes
    glUseProgram(grid_shader_->program);
    GLint u_mvp = glGetUniformLocation(grid_shader_->program, "u_mvp");
    GLint u_color = glGetUniformLocation(grid_shader_->program, "u_color");
    GLint u_color_mod = glGetUniformLocation(grid_shader_->program, "u_color_mod");
    GLint u_size_scale = glGetUniformLocation(grid_shader_->program, "u_size_scale");
    GLint u_center_mod = glGetUniformLocation(grid_shader_->program, "u_center_mod");
    
    //set uniforms and draw grid
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, vp.m);
    glUniform3fv(u_color, 4, grid_colors);
    glUniform3f(u_size_scale, 1.0, 1.0, 1.0);
    glUniform3f(u_center_mod, 0.0, 0.0, 0.0);
    glUniform1i(u_color_mod, 0);
    glBindVertexArray(grid_vao_); //GRID
    glDrawElements(GL_LINES, grid_num_indices, GL_UNSIGNED_INT, 0);
}

void DebugSystem::drawFrusta_() {
    //get the camera view projection matrix
    lm::mat4 vp = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
    GLint u_mvp = glGetUniformLocation(grid_shader_->program, "u_mvp");
    GLint u_color_mod = glGetUniformLocation(grid_shader_->program, "u_color_mod");
    
    //draw frustra for all cameras
    auto& cameras = ECS.getAllComponents<Camera>();
    int counter = 0;
    for (auto& cc : cameras) {
        //don't draw current camera frustum
        if (counter == ECS.main_camera) continue;
        counter++;
        
        lm::mat4 cam_iv = cc.view_matrix;
        cam_iv.inverse();
        lm::mat4 cam_ip = cc.projection_matrix;
        cam_ip.inverse();
        lm::mat4 cam_ivp = cc.view_projection;
        cam_ivp.inverse();
        lm::mat4 mvp = vp * cam_ivp;
        
        //set uniforms and draw cube
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
        glUniform1i(u_color_mod, 1); //set color to index 1 (red)
        glBindVertexArray(cube_vao_); //CUBE
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    }
}

void DebugSystem::drawColliders_() {
    //get the camera view projection matrix
    lm::mat4 vp = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
    GLint u_mvp = glGetUniformLocation(grid_shader_->program, "u_mvp");
    GLint u_color_mod = glGetUniformLocation(grid_shader_->program, "u_color_mod");
    
    //draw all colliders
    auto& colliders = ECS.getAllComponents<Collider>();
    for (auto& cc : colliders) {
        //get transform for collider
        Transform& tc = ECS.getComponentFromEntity<Transform>(cc.owner);
        //get the colliders local model matrix in order to draw correctly
        lm::mat4 collider_matrix = tc.getGlobalMatrix(ECS.getAllComponents<Transform>());
        
        if (cc.collider_type == ColliderTypeBox) {
            
            //now move by the box by its offset
            collider_matrix.translateLocal(cc.local_center.x, cc.local_center.y, cc.local_center.z);
            //convert -1 -> +1 geometry to size of collider box
            collider_matrix.scaleLocal(cc.local_halfwidth.x, cc.local_halfwidth.y, cc.local_halfwidth.z);
            //set mvp
            lm::mat4 mvp = vp * collider_matrix;
            
            //set uniforms and draw
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
            glUniform1i(u_color_mod, 2); //set color to index 2 (green)
            glBindVertexArray(cube_vao_); //CUBE
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        }
        
        if (cc.collider_type == ColliderTypeRay) {
            //ray natively goes from (0,0,0 to 0,0,1) (see definition in createRay_())
            //we need to rotate this vector so that it matches the direction specified by the component
            //to do this we first need to find angle and axis between the two vectors
            lm::vec3 buffer_vec(0, 0, 1);
            lm::vec3 dir_norm = cc.direction;
            dir_norm.normalize();
            float rotation_angle = acos(dir_norm.dot(buffer_vec));
            //if angle is PI, vector is opposite to buffer vec
            //so rotation axis can be anything (we set it to 0,1,0 vector
            lm::vec3 rotation_axis = lm::vec3(0, 1, 0);
            //otherwise, we calculate rotation axis with cross product
            if (rotation_angle < 3.14159f) {
                rotation_axis = dir_norm.cross(buffer_vec).normalize();
            }
            //now we rotate the buffer vector to
            if (rotation_angle > 0.00001f) {
                //only rotate if we have to
                collider_matrix.rotateLocal(rotation_angle, rotation_axis);
            }
            //apply distance scale
            collider_matrix.scaleLocal(cc.max_distance, cc.max_distance, cc.max_distance);
            //apply center offset
            collider_matrix.translateLocal(cc.local_center.x, cc.local_center.y, cc.local_center.z);
            
            //set uniforms
            lm::mat4 mvp = vp * collider_matrix;
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
            //set color to index 2 (green)
            glUniform1i(u_color_mod, 3);
            
            //bind the cube vao
            glBindVertexArray(collider_ray_vao_);
            glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0);
        }
    }
}

void DebugSystem::drawIcons_() {
    lm::mat4 vp = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
    
    //switch to icon shader
    glUseProgram(icon_shader_->program);
    
    //get uniforms
    GLint u_mvp = glGetUniformLocation(icon_shader_->program, "u_mvp");
    GLint u_icon = glGetUniformLocation(icon_shader_->program, "u_icon");
    glUniform1i(u_icon, 0);
    
    
    //for each light - bind light texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, icon_light_texture_);
    
    auto& lights = ECS.getAllComponents<Light>();
    for (auto& curr_light : lights) {
        Transform& curr_light_transform = ECS.getComponentFromEntity<Transform>(curr_light.owner);
        
        lm::mat4 mvp_matrix = vp * curr_light_transform.getGlobalMatrix(ECS.getAllComponents<Transform>());;
        //BILLBOARDS
        //the mvp for the light contains rotation information. We want it to look at the camera always.
        //So we zero out first three columns of matrix, which contain the rotation information
        //this is an extremely simple billboard
        lm::mat4 bill_matrix;
        for (int i = 12; i < 16; i++) bill_matrix.m[i] = mvp_matrix.m[i];
        
        //send this new matrix as the MVP
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, bill_matrix.m);
        glBindVertexArray(icon_vao_);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    
    //bind camera texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, icon_camera_texture_);
    
    //for each camera, exactly the same but with camera texture
    auto& cameras = ECS.getAllComponents<Camera>();
    for (auto& curr_camera : cameras) {
        Transform& curr_cam_transform = ECS.getComponentFromEntity<Transform>(curr_camera.owner);
        lm::mat4 mvp_matrix = vp * curr_cam_transform.getGlobalMatrix(ECS.getAllComponents<Transform>());
        
        // billboard as above
        lm::mat4 bill_matrix;
        for (int i = 12; i < 16; i++) bill_matrix.m[i] = mvp_matrix.m[i];
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, bill_matrix.m);
        glBindVertexArray(icon_vao_);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
    }
}

///////////////////////////////////////////////
// **** Functions to create geometry ********//
///////////////////////////////////////////////

//creates a simple quad to store a light texture
void DebugSystem::createIcon_() {
	float is = 0.5f;
	GLfloat icon_vertices[12]{ -is, -is, 0, is, -is, 0, is, is, 0, -is, is, 0 };
	GLfloat icon_uvs[8]{ 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
	GLuint icon_indices[6]{ 0, 1, 2, 0, 2, 3 };
	glGenVertexArrays(1, &icon_vao_);
	glBindVertexArray(icon_vao_);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(icon_vertices), icon_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//uvs
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(icon_uvs), icon_uvs, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(icon_indices), icon_indices, GL_STATIC_DRAW);
	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void DebugSystem::createRay_() {
	//4th component is color
	GLfloat icon_vertices[8]{ 0, 0, 0, 0,
		0, 0, 1, 0 };
	GLuint icon_indices[2]{ 0, 1 };
	glGenVertexArrays(1, &collider_ray_vao_);
	glBindVertexArray(collider_ray_vao_);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(icon_vertices), icon_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(icon_indices), icon_indices, GL_STATIC_DRAW);
	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void DebugSystem::createCube_() {

	//4th component is color!
	const GLfloat quad_vertex_buffer_data[] = {
		-1.0f,  -1.0f,  -1.0f,  0.0f,  //near bottom left
		1.0f,   -1.0f,  -1.0f,  0.0f,   //near bottom right
		1.0f,   1.0f,   -1.0f,  0.0f,    //near top right
		-1.0f,  1.0f,   -1.0f,  0.0f,   //near top left
		-1.0f,  -1.0f,  1.0f,   0.0f,   //far bottom left
		1.0f,   -1.0f,  1.0f,   0.0f,    //far bottom right
		1.0f,   1.0f,   1.0f,   0.0f,     //far top right
		-1.0f,  1.0f,   1.0f,   0.0f,    //far top left
	};

	const GLuint quad_index_buffer_data[] = {
		0,1, 1,2, 2,3, 3,0, //top
		4,5, 5,6, 6,7, 7,4, // bottom
		4,0, 7,3, //left
		5,1, 6,2, //right
	};

	glGenVertexArrays(1, &cube_vao_);
	glBindVertexArray(cube_vao_);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_buffer_data), quad_vertex_buffer_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_index_buffer_data), quad_index_buffer_data, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//creates the debug grid for our scene
void DebugSystem::createGrid_() {

	std::vector<float> grid_vertices;
	const float size = 100.0f; //outer width and height
	const int div = 100; // how many divisions
	const int halfdiv = div / 2;
	const float step = size / div; // gap between divisions
	const float half = size / 2; // middle of grid

	float p; //temporary variable to store position
	for (int i = 0; i <= div; i++) {

		//lines along z-axis, need to vary x position
		p = -half + (i*step);
		//one end of line
		grid_vertices.push_back(p);
		grid_vertices.push_back(0);
		grid_vertices.push_back(half);
		if (i == halfdiv) grid_vertices.push_back(1); //color
		else grid_vertices.push_back(0);

		//other end of line
		grid_vertices.push_back(p);
		grid_vertices.push_back(0);
		grid_vertices.push_back(-half);
		if (i == halfdiv) grid_vertices.push_back(1); //color
		else grid_vertices.push_back(0);

		//lines along x-axis, need to vary z positions
		p = half - (i * step);
		//one end of line
		grid_vertices.push_back(-half);
		grid_vertices.push_back(0);
		grid_vertices.push_back(p);
		if (i == halfdiv) grid_vertices.push_back(3); //color
		else grid_vertices.push_back(0);

		//other end of line
		grid_vertices.push_back(half);
		grid_vertices.push_back(0);
		grid_vertices.push_back(p);
		if (i == halfdiv) grid_vertices.push_back(3); //color
		else grid_vertices.push_back(0);
	}

	//indices
	const int num_indices = (div + 1) * 4;
	GLuint grid_line_indices[num_indices];
	for (int i = 0; i < num_indices; i++)
		grid_line_indices[i] = i;

	grid_num_indices = num_indices;

	//gl buffers
	glGenVertexArrays(1, &grid_vao_);
	glBindVertexArray(grid_vao_);
	GLuint vbo;
	//positions
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, grid_vertices.size() * sizeof(float), &(grid_vertices[0]), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	//indices
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(grid_line_indices), grid_line_indices, GL_STATIC_DRAW);

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

