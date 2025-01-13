#include <list>
#include "frontUI.hpp"
#include <memory>
#include <iostream>
#include "../state/window.hpp"
#include "../state/state.hpp"
#include "../state/cursor.hpp"
#include "../state/time.hpp"
#include "../state/camera.hpp"

using namespace std;
using namespace Game;

static glm::vec4 scoreColor(1, 1, 0, 1);
static glm::vec4 startColor(0, 1, 0, 1);
static glm::vec4 exitColor(1, 0, 0, 1);
static glm::vec4 restartColor(1, 1, 0, 1);

static UI scoreBoard;
static UI largeScoreBoard;
static UI startButtom;
static UI exitButton;
static UI restartButton;
static UI redCrosses[3];
static UI emptyCrosses[3];

static unique_ptr<Shader> trailShader;
static GLuint trailTexture;
static GLuint trailArrow;
static list<glm::vec3> mousePositions;
static GLuint mouseTrailVAO;
static GLuint mouseTrailVBO;
static GLuint mouseTrailEBO;


// Given three collinear points p, q, r, the function checks if 
// point q lies on line segment 'pr' 
bool onSegment(glm::vec2 p, glm::vec2 q, glm::vec2 r)
{
	if (q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) &&
		q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y))
		return true;

	return false;
}

// To find orientation of ordered triplet (p, q, r). 
// The function returns following values 
// 0 --> p, q and r are collinear 
// 1 --> Clockwise 
// 2 --> Counterclockwise 
int orientation(glm::vec2 p, glm::vec2 q, glm::vec2 r)
{
	// See https://www.geeksforgeeks.org/orientation-3-ordered-points/ 
	// for details of below formula. 
	int val = (q.y - p.y) * (r.x - q.x) -
		(q.x - p.x) * (r.y - q.y);

	if (val == 0) return 0;  // collinear 

	return (val > 0) ? 1 : 2; // clock or counterclock wise 
}

// The main function that returns true if line segment 'p1q1' 
// and 'p2q2' intersect. 
bool doIntersect(glm::vec2 p1, glm::vec2 q1, glm::vec2 p2, glm::vec2 q2)
{
	// Find the four orientations needed for general and 
	// special cases 
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);

	// General case 
	if (o1 != o2 && o3 != o4)
		return true;

	// Special Cases 
	// p1, q1 and p2 are collinear and p2 lies on segment p1q1 
	if (o1 == 0 && onSegment(p1, p2, q1)) return true;

	// p1, q1 and q2 are collinear and q2 lies on segment p1q1 
	if (o2 == 0 && onSegment(p1, q2, q1)) return true;

	// p2, q2 and p1 are collinear and p1 lies on segment p2q2 
	if (o3 == 0 && onSegment(p2, p1, q2)) return true;

	// p2, q2 and q1 are collinear and q1 lies on segment p2q2 
	if (o4 == 0 && onSegment(p2, q1, q2)) return true;

	return false; // Doesn't fall in any of the above cases 
}

static void OnMouseUpdate(glm::vec2 position) {
	if (Game::mouseClicked) {
		glm::vec2 expanded(position.x * SCR_WIDTH / 2, position.y * SCR_HEIGHT / 2);
		if (!mousePositions.empty()) {
			glm::vec2 lastPos = mousePositions.back();
			if (glm::length(expanded - lastPos) < 12) {
				return;
			}
		}
		
		mousePositions.push_back(glm::vec3(expanded, POS_KEEP_TIME));
		if (mousePositions.size() > MAX_MOUSE_POS_SIZE) {
			mousePositions.pop_front();
		}
	}
}

void initFrontUI() {
	scoreBoard = UI(-1, "Score: ");
	largeScoreBoard = UI(-1, "Score", 70);
	startButtom = UI(-1, "Start");
	exitButton = UI(-1, "Exit");
	restartButton = UI(-1, "Restart");
	restartButton.transform.SetPosition(FINAL_SCORE_POS);

	GLuint emptyCross = textureFromFile("empty_cross.png", "images");
	GLuint redCross = textureFromFile("red_cross.png", "images");
	for (auto i = 0; i < 3;i++) {
		redCrosses[i] = UI(redCross);
		emptyCrosses[i] = UI(emptyCross);
		glm::mat4 rotation = glm::rotate(glm::mat4(1), (float)glm::radians(10 + 10.0f * (2 - i)), glm::vec3(0, 0, -1));
		redCrosses[i].transform.SetRotation(rotation);
		redCrosses[i].transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
		emptyCrosses[i].transform.SetRotation(rotation);
		emptyCrosses[i].transform.SetScale(glm::vec3(0.05, 0.05, 0.05));
	}

	trailShader = make_unique<Shader>("shaders/mouse_trail.vert", "shaders/mouse_trail.frag");
	trailTexture = textureFromFile("FruitNinja_blade0.png", "images");
	trailArrow = textureFromFile("blade0_arrow.png", "images");
	Game::OnMousePositionUpdated.push_back(OnMouseUpdate);
	glGenBuffers(1, &mouseTrailVBO);
	glGenBuffers(1, &mouseTrailEBO);
	glGenVertexArrays(1, &mouseTrailVAO);

	glBindVertexArray(mouseTrailVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mouseTrailVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mouseTrailEBO);

	glBufferData(GL_ARRAY_BUFFER, 4 * MAX_MOUSE_POS_SIZE * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_MOUSE_POS_SIZE * 6 * sizeof(int), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)sizeof(glm::vec3));
	glBindVertexArray(0);
}

static void insertTrailArrow(char* currVBO, int* currEBO, glm::vec2 left, glm::vec2 right, glm::vec2 tangent, int index) {
	glm::vec3 backLeftPos(left, -0.8f);
	glm::vec3 backRightPos(right, -0.8f);
	glm::vec2 backLeftUV(0, 1);
	memcpy(currVBO, glm::value_ptr(backLeftPos), sizeof(backLeftPos));
	currVBO += sizeof(glm::vec3);
	memcpy(currVBO, glm::value_ptr(backLeftUV), sizeof(backLeftUV));
	currVBO += sizeof(glm::vec2);

	glm::vec2 backRightUV(0, 0);
	memcpy(currVBO, glm::value_ptr(backRightPos), sizeof(backRightPos));
	currVBO += sizeof(glm::vec3);
	memcpy(currVBO, glm::value_ptr(backRightUV), sizeof(backRightUV));
	currVBO += sizeof(glm::vec2);

	auto offset = TRAIL_ARROW_SIZE * glm::normalize(glm::vec3(tangent, 0));

	glm::vec3 frontLeftPos = backLeftPos + offset;
	glm::vec2 frontLeftUV(1, 1);
	memcpy(currVBO, glm::value_ptr(frontLeftPos), sizeof(frontLeftPos));
	currVBO += sizeof(glm::vec3);
	memcpy(currVBO, glm::value_ptr(frontLeftUV), sizeof(frontLeftUV));
	currVBO += sizeof(glm::vec2);

	glm::vec3 frontRightPos = backRightPos + offset;
	glm::vec2 frontRightUV(1, 0);
	memcpy(currVBO, glm::value_ptr(frontRightPos), sizeof(frontRightPos));
	currVBO += sizeof(glm::vec3);
	memcpy(currVBO, glm::value_ptr(frontRightUV), sizeof(frontRightUV));
	currVBO += sizeof(glm::vec2);
	currEBO[0] = index;
	currEBO[1] = index + 1;
	currEBO[2] = index + 2;
	currEBO[3] = index + 3;
	currEBO[4] = index + 2;
	currEBO[5] = index + 1;
}

static void drawMouseTrail() {
	float dt = deltaTime();
	for (auto i = mousePositions.begin(); i != mousePositions.end();) {
		i->z -= dt;
		if (i->z <= 0) {
			i = mousePositions.erase(i);
		}
		else {
			i++;
		}
	}

	glBindVertexArray(mouseTrailVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mouseTrailVBO);
	if (mousePositions.size() > 1) {
		if (mousePositions.size() > 10) {
			cout << "";
		}
		char* currVBO = (char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		int* currEBO = (int*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

		auto prev = mousePositions.begin();
		auto curr = next(prev, 1);
		glm::vec2 tangent{};
		int index = 0;
		int indices = 6;
		glm::vec3 backLeftPos;
		glm::vec3 backRightPos;
		bool init = true;
		while (curr != mousePositions.end()) {
			glm::vec2 prevPos(*prev);
			glm::vec2 currPos(*curr);
			tangent = currPos - prevPos;

			glm::vec2 right = glm::normalize(glm::vec2(tangent.y, -tangent.x));
			float prevLineSize = prev->z * MAX_TRAIL_SIZE / POS_KEEP_TIME;
			float currLineSize = curr->z * MAX_TRAIL_SIZE / POS_KEEP_TIME;

			glm::vec2 prevRight = prevLineSize * right;
			glm::vec2 prevLeft = -prevLineSize * right;
			glm::vec2 currRight = currLineSize * right;
			glm::vec2 currLeft = -currLineSize * right;
			
			if (init) {
				init = false;
				backLeftPos = glm::vec3(prevPos + prevLeft, -0.5);
				backRightPos = glm::vec3(prevPos + prevRight, -0.5);
			}
			
			glm::vec2 backLeftUV(0, 1);
			memcpy(currVBO, glm::value_ptr(backLeftPos), sizeof(backLeftPos));
			currVBO += sizeof(glm::vec3);
			memcpy(currVBO, glm::value_ptr(backLeftUV), sizeof(backLeftUV));
			currVBO += sizeof(glm::vec2);

			glm::vec2 backRightUV(0, 0);
			memcpy(currVBO, glm::value_ptr(backRightPos), sizeof(backRightPos));
			currVBO += sizeof(glm::vec3);
			memcpy(currVBO, glm::value_ptr(backRightUV), sizeof(backRightUV));
			currVBO += sizeof(glm::vec2);

			glm::vec3 frontLeftPos(currPos + currLeft, -0.5);
			glm::vec2 frontLeftUV(1, 1);
			memcpy(currVBO, glm::value_ptr(frontLeftPos), sizeof(frontLeftPos));
			currVBO += sizeof(glm::vec3);
			memcpy(currVBO, glm::value_ptr(frontLeftUV), sizeof(frontLeftUV));
			currVBO += sizeof(glm::vec2);

			glm::vec3 frontRightPos(currPos + currRight, -0.5);
			glm::vec2 frontRightUV(1, 0);
			memcpy(currVBO, glm::value_ptr(frontRightPos), sizeof(frontRightPos));
			currVBO += sizeof(glm::vec3);
			memcpy(currVBO, glm::value_ptr(frontRightUV), sizeof(frontRightUV));
			currVBO += sizeof(glm::vec2);

			if (doIntersect(backLeftPos, backRightPos, frontLeftPos, frontRightPos)) {
				currEBO[0] = index;
				currEBO[1] = index + 1;
				currEBO[2] = index + 2;
				currEBO[3] = index + 1;
				currEBO[4] = index;
				currEBO[5] = index + 3;
			}
			else {
				currEBO[0] = index;
				currEBO[1] = index + 1;
				currEBO[2] = index + 2;
				currEBO[3] = index + 3;
				currEBO[4] = index + 2;
				currEBO[5] = index + 1;
			}
			backLeftPos = frontLeftPos;
			backRightPos = frontRightPos;

			currEBO += 6;
			indices += 6;
			curr++;
			prev++;
			index += 4;
		}
		insertTrailArrow(currVBO, currEBO, backLeftPos, backRightPos, tangent, index);

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, trailTexture);
		trailShader->SetInt("image", 0);
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, trailArrow);
		trailShader->SetInt("arrow", 1);
		glUniformMatrix4fv(glGetUniformLocation(trailShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(ortho));
		glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_INT, 0);
	}
	
	glBindVertexArray(0);
}

void drawFrontUI(Shader& shader) {
	
	float halfWidth = SCR_WIDTH / 2.0f;
	float halfHeight = SCR_HEIGHT / 2.0f;
	if (Game::state == State::START) {
		shader.SetVec4("textColor", startColor);
		startButtom.DrawInNDC(START_BUTTON_POS, shader);
		shader.SetVec4("textColor", exitColor);
		exitButton.DrawInNDC(EXIT_BUTTON_POS, shader);
	}
	else if (Game::state == State::GAME) {
		shader.SetVec4("textColor", scoreColor);
		scoreBoard.transform.SetPosition(glm::vec3(-0.87 * halfWidth, 0.9 * halfHeight, 0));
		scoreBoard.UpdateText("Score: " + to_string(score));
		scoreBoard.Draw(shader);

		for (int i = 0; i < 3;i++) {
			if (i < Game::misses) {
				redCrosses[i].transform.SetPosition(glm::vec3((0.95 - i * 0.06) * halfWidth, (0.8 + (i + 1) * 0.04) * halfHeight, 0));
				redCrosses[i].Draw(shader);
			}
			else {
				emptyCrosses[i].transform.SetPosition(glm::vec3((0.95 - i * 0.06) * halfWidth, (0.8 + (i + 1) * 0.04) * halfHeight, 0));
				emptyCrosses[i].Draw(shader);
			}
		}
	}
	else {
		shader.SetVec4("textColor", restartColor);
		restartButton.DrawInNDC(START_BUTTON_POS, shader);

		shader.SetVec4("textColor", exitColor);
		exitButton.DrawInNDC(EXIT_BUTTON_POS, shader);

		shader.SetVec4("textColor", scoreColor);
		largeScoreBoard.UpdateText("Score: " + to_string(score));
		largeScoreBoard.Draw(shader);
	}
	trailShader->Use();
	drawMouseTrail();
	shader.Use();
}