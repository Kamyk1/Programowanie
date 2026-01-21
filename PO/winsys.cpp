#include "cpoint.h"
#include "screen.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <ctype.h>
#include <list>
#include <ncurses.h>
#include <string.h>
#include <string>
#include <time.h>
#include <vector>

using namespace std;

class CView {
  protected:
	CRect geom;

  public:
	CView(CRect g) : geom(g){};
	virtual void paint() = 0;
	virtual bool handleEvent(int key) = 0;
	virtual void move(const CPoint& delta)
	{
		geom.topleft += delta;
	};
	virtual ~CView(){};
};

class CWindow : public CView {
  protected:
	char c;

  public:
	CWindow(CRect r, char _c = '*') : CView(r), c(_c){};
	void paint()
	{
		for (int i = geom.topleft.y; i < geom.topleft.y + geom.size.y; i++) {
			gotoyx(i, geom.topleft.x);
			for (int j = 0; j < geom.size.x; j++)
				printw("%c", c);
		};
	};
	bool handleEvent(int key)
	{
		switch (key) {
		case KEY_UP:
			move(CPoint(0, -1));
			return true;
		case KEY_DOWN:
			move(CPoint(0, 1));
			return true;
		case KEY_RIGHT:
			move(CPoint(1, 0));
			return true;
		case KEY_LEFT:
			move(CPoint(-1, 0));
			return true;
		};
		return false;
	};
};

class CFramedWindow : public CWindow {
  public:
	CFramedWindow(CRect r, char _c = '\'') : CWindow(r, _c){};
	void paint()
	{
		for (int i = geom.topleft.y; i < geom.topleft.y + geom.size.y; i++) {
			gotoyx(i, geom.topleft.x);
			if ((i == geom.topleft.y) ||
				(i == geom.topleft.y + geom.size.y - 1)) {
				printw("+");
				for (int j = 1; j < geom.size.x - 1; j++)
					printw("-");
				printw("+");
			}
			else {
				printw("|");
				for (int j = 1; j < geom.size.x - 1; j++)
					printw("%c", c);
				printw("|");
			}
		}
	};
};

class CMenuWindow : public CFramedWindow {
  public:
	CMenuWindow(CRect r) : CFramedWindow(r, ' '){};

	void paint() override
	{
		CFramedWindow::paint();
		int midX = geom.topleft.x + (geom.size.x / 2) - 5;
		int startY = geom.topleft.y + 2;

		gotoyx(startY, midX);
		printw("  S - START  ");
		gotoyx(startY + 2, midX);
		printw("  O - OPTIONS");
		gotoyx(startY + 4, midX);
		printw("  H - HELP   ");

		gotoyx(startY + 7, midX - 2);
		printw("Press ESC to exit");
	}

	bool handleEvent(int key) override
	{
		if (CFramedWindow::handleEvent(key))
			return true;

		switch (tolower(key)) {
		case 's':
			return true;
		case 'o':
			return true;
		case 'h':
			return true;
		}
		return false;
	}
};

enum GameState { MENU, PLAYING, HELP, GAMEOVER, SCORETABLE, PAUSE };

enum DIRECTION { UP, LEFT, RIGHT, DOWN };

class CSnakeWindow : public CFramedWindow {
  private:
	int baseSpeed = 150;
	int minSpeed = 40;
	int speedStep = 10;
	int levelStep = 5;
	vector<CPoint> snake;
	CPoint food;
	GameState state;
	GameState previousstate;
	DIRECTION currentDir;
	int levelcount = 0;
	int highscore = 0;
	int scoreboard[10];
	bool onboard = false;
	string usernames[10] = {"SnakeCore", "PixelRun", "NeoWindow", "TermiPlay",
							"GridMove",	 "FrameX",	 "ByteSnake", "CurWin",
							"CodeField", "SnakeLab"};

  public:
	CSnakeWindow(CRect r, char _c = ' ')
		: CFramedWindow(r, _c), state(MENU), previousstate(MENU),
		  currentDir(RIGHT)
	{
		initScoreBoard();
	};
	void paint() override
	{
		if (state == PLAYING) {
			moveSnake();
		}
		CFramedWindow::paint();
		int midX = geom.topleft.x + (geom.size.x / 2);
		int midY = geom.topleft.y + (geom.size.y / 2);
		switch (state) {
		case MENU:
			gotoyx(midY - 2, midX - 5);
			printw("=== SNAKE ===");
			gotoyx(midY, midX - 7);
			printw("S - Start Game");
			gotoyx(midY + 1, midX - 7);
			printw("O - ScoreBoard");
			gotoyx(midY + 2, midX - 7);
			printw("H - How to play");
			break;
		case PLAYING:
			gotoyx(geom.topleft.y - 1, geom.topleft.x);
			printw("| LEVEL: %3d |", levelcount);
			gotoyx(geom.topleft.y - 1, geom.topleft.x + 51 - 9);
			printw("| HIGHSCORE: %3d |", highscore);
			gotoyx(geom.topleft.y + food.y, geom.topleft.x + food.x);
			printw("*");
			for (size_t i = 0; i < snake.size(); i++) {
				gotoyx(geom.topleft.y + snake[i].y,
					   geom.topleft.x + snake[i].x);
				if (i == snake.size() - 1) {
					printw("@");
				}
				else {
					printw("+");
				}
			}
			break;
		case HELP:
			gotoyx(midY - 2, midX - 6);
			printw("=== HELPER ===");
			gotoyx(midY, midX - 11);
			printw("Arrows: Move | P: Pause");
			gotoyx(midY + 1, midX - 10);
			printw("R: Restart | H: Help");
			gotoyx(midY + 4, midX - 10);
			printw("Press 'B' to return");
			break;
		case GAMEOVER:
			gotoyx(midY, midX - 5);
			printw("GAME OVER");
			gotoyx(midY + 4, midX - 10);
			printw("Press 'R' to restart");
			gotoyx(midY + 9, midX - 16);
			printw("Press 'B' to return to main menu");
			break;
		case SCORETABLE: {
			int centerX = geom.topleft.x + (geom.size.x / 2);
			int centerY = geom.topleft.y + (geom.size.y / 2);
			gotoyx(centerY - 7, centerX - 6);
			printw("=== TOP 10 ===");
			int startX = centerX - 12;
			int startY = centerY - 5;
			for (int i = 0; i < 10; i++) {
				gotoyx(startY + i, startX);
				printw("%2d. %-10s : %4d pkt", (int)i + 1, usernames[i].c_str(),
					   scoreboard[i]);
			}
			gotoyx(centerY + 6, centerX - 12);

			if (!onboard) {
				printw("xx. YOU        : %4d", highscore);
			}
			gotoyx(centerY + 9, centerX - 10);
			printw("Press 'B' to return");
		} break;
		case PAUSE:
			gotoyx(midY, midX - 2);
			printw("PAUSE");
			gotoyx(midY + 9, midX - 16);
			printw("Press 'B' to return to main menu");
			break;
		}
	}
	bool handleEvent(int c)
	{
		switch (state) {
		case MENU:
			if (tolower(c) == 's') {
				initSnake();
				state = PLAYING;
				timeout(150);
				return true;
			}
			if (tolower(c) == 'o') {
				previousstate = state;
				state = SCORETABLE;
				return true;
			}
			if (tolower(c) == 'h') {
				previousstate = state;
				state = HELP;
				return true;
			}
			break;
		case SCORETABLE:
			if (tolower(c) == 'b') {
				previousstate = state;
				state = MENU;
				return true;
			}
			break;
		case PLAYING:
			switch (c) {
			case KEY_UP:
				if (currentDir != DOWN)
					currentDir = UP;
				return true;
			case KEY_DOWN:
				if (currentDir != UP)
					currentDir = DOWN;
				return true;
			case KEY_RIGHT:
				if (currentDir != LEFT)
					currentDir = RIGHT;
				return true;
			case KEY_LEFT:
				if (currentDir != RIGHT)
					currentDir = LEFT;
				return true;
			case 'h':
				previousstate = PLAYING;
				state = HELP;
				break;
			case 'p':
				previousstate = PLAYING;
				state = PAUSE;
				break;
			}
		case HELP:
			if (tolower(c) == 'b') {
				state = previousstate;
			}
			return true;
			break;
		case GAMEOVER:
			if (tolower(c) == 'r') {
				previousstate = state;
				resetGame();
				state = PLAYING;
			}
			if (tolower(c) == 'b') {
				state = MENU;
				previousstate = MENU;
			}
			break;
		case PAUSE:
			if (tolower(c) == 'b') {
				state = PLAYING;
				previousstate = PAUSE;
			}
			break;
			return true;
		}
		if (CFramedWindow::handleEvent(c))
			return true;
		return false;
	}
	bool addToScoreboard(int score)
	{
		if (score > highscore)
			highscore = score;
		for (int i = 0; i < 10; i++) {
			if (score > scoreboard[i]) {
				for (int j = 9; j > i; j--) {
					scoreboard[j] = scoreboard[j - 1];
					usernames[j] = usernames[j - 1];
				}
				scoreboard[i] = score;
				usernames[i] = "YOU";
				onboard = true;
				return true;
			}
		}
		return false;
	}
	void initScoreBoard()
	{
		for (int i = 0; i < 10; i++) {
			scoreboard[i] = 10 + (rand() % 191);
		}
		for (int i = 0; i < 9; i++) {
			for (int j = 0; j < 9 - i; j++) {
				if (scoreboard[j] < scoreboard[j + 1]) {
					int tempScore = scoreboard[j];
					scoreboard[j] = scoreboard[j + 1];
					scoreboard[j + 1] = tempScore;
					string tempName = usernames[j];
					usernames[j] = usernames[j + 1];
					usernames[j + 1] = tempName;
				}
			}
		}
	}
	void resetGame()
	{
		onboard = false;
		levelcount = 0;
		currentDir = RIGHT;
		initSnake();
		timeout(baseSpeed);
	}
	void initSnake()
	{
		snake.clear();
		levelcount = 0;
		snake.push_back(CPoint(10, 10));
		snake.push_back(CPoint(10, 11));
		snake.push_back(CPoint(10, 12));
		currentDir = RIGHT;
		spawnFood();
		timeout(baseSpeed);
	}
	void moveSnake()
	{
		CPoint newHead = snake.back();
		if (currentDir == UP)
			newHead.y--;
		if (currentDir == DOWN)
			newHead.y++;
		if (currentDir == LEFT)
			newHead.x--;
		if (currentDir == RIGHT)
			newHead.x++;

		if (newHead.x <= 0) {
			newHead.x = 59;
			currentDir = LEFT;
		}
		else if (newHead.x >= geom.size.x - 1) {
			newHead.x = 0;
			currentDir = RIGHT;
		}
		if (newHead.y <= 0) {
			newHead.y = 24;
			currentDir = UP;
		}
		else if (newHead.y >= geom.size.y - 1) {
			newHead.y = 0;
			currentDir = DOWN;
		}
		if (newHead.x == food.x && newHead.y == food.y) {
			snake.push_back(newHead);
			levelcount++;
			updateSpeed();
			spawnFood();
		}
		else {
			snake.push_back(newHead);
			snake.erase(snake.begin());
		}
		for (size_t i = 0; i < (size_t)levelcount + 2; i++) {
			if (newHead.x == snake[i].x && newHead.y == snake[i].y) {
				state = GAMEOVER;
				addToScoreboard(levelcount);
				return;
			}
		}
	}
	void spawnFood()
	{
		bool onSnake;
		do {
			onSnake = false;
			food.x = 1 + (rand() % (geom.size.x - 2));
			food.y = 1 + (rand() % (geom.size.y - 2));
			for (size_t i = 0; i < (size_t)levelcount + 2; i++) {
				if (snake[i].x == food.x && snake[i].y == food.y) {
					onSnake = true;
					break;
				}
			}
		} while (onSnake);
	}
	void updateSpeed()
	{
		int newSpeed = baseSpeed - (levelcount / levelStep) * speedStep;
		if (newSpeed < minSpeed)
			newSpeed = minSpeed;
		timeout(newSpeed);
	}
};

class CInputLine : public CFramedWindow {
	string text;

  public:
	CInputLine(CRect r, char _c = ',') : CFramedWindow(r, _c){};
	void paint()
	{
		CFramedWindow::paint();
		gotoyx(geom.topleft.y + 1, geom.topleft.x + 1);

		for (unsigned j = 1, i = 0;
			 (j + 1 < (unsigned)geom.size.x) && (i < text.length()); j++, i++)
			printw("%c", text[i]);
	};
	bool handleEvent(int c)
	{
		if (CFramedWindow::handleEvent(c))
			return true;
		if ((c == KEY_DC) || (c == KEY_BACKSPACE)) {
			if (text.length() > 0) {
				text.erase(text.length() - 1);
				return true;
			};
		}
		if ((c > 255) || (c < 0))
			return false;
		if (!isalnum(c) && (c != ' '))
			return false;
		text.push_back(c);
		return true;
	}
};

class CGroup : public CView {
	list<CView*> children;

  public:
	CGroup(CRect g) : CView(g){};
	void paint()
	{
		for (CView* i : children)
			i->paint();
	};
	bool handleEvent(int key)
	{
		if (!children.empty() && children.back()->handleEvent(key))
			return true;
		if (key == '\t') {
			if (!children.empty()) {
				children.push_front(children.back());
				children.pop_back();
			};
			return true;
		}
		return false;
	};
	void insert(CView* v)
	{
		children.push_back(v);
	};
	~CGroup()
	{
		for (CView* i : children)
			delete i;
	};
};

class CDesktop : public CGroup {
  public:
	CDesktop() : CGroup(CRect())
	{
		int y, x;
		init_screen();
		getscreensize(y, x);
		geom.size.x = x;
		geom.size.y = y;
	};
	~CDesktop()
	{
		done_screen();
	};

	void paint()
	{
		for (int i = geom.topleft.y; i < geom.topleft.y + geom.size.y; i++) {
			gotoyx(i, geom.topleft.x);
			for (int j = 0; j < geom.size.x; j++)
				printw(".");
		};
		CGroup::paint();
	}

	int getEvent()
	{
		return ngetch();
	};

	void run()
	{
		int c;

		timeout(150);
		paint();
		refresh();
		while (1) {
			c = getEvent();
			if (c == 27) // ESC key
				break;
			if (handleEvent(c) || c == -1) {
				paint();
				refresh();
			};
		};
	};
};

int main()
{
	srand(time(nullptr));
	CDesktop d;
	d.insert(new CInputLine(CRect(CPoint(5, 7), CPoint(15, 15))));
	d.insert(new CWindow(CRect(CPoint(2, 3), CPoint(20, 10)), '#'));
	d.insert(new CSnakeWindow(CRect(CPoint(40, 4), CPoint(60, 25))));
	d.run();
	return 0;
}
