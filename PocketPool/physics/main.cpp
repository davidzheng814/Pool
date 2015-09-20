#include <cstdio>
#include <cstdlib>
#include <complex>
#include <math.h>

typedef std::complex<double> vector;

#define DEFAULT_TIME_STEP 0.04
#define X real()
#define Y imag()

const double CORNER_WITHIN_WALL = .03; //perp distance between center of pocket and the rail
const double SIDE_WITHIN_WALL = .05;
const double CORNER_WALL_MISSING = .04; //amount of wall that's missing in each direction around the corner
const double SIDE_WALL_MISSING = .05; //amount of wall missing on either side around the side
const double CORNER_SLOPE = 1; //slope of the top left corner wall
const double SIDE_SLOPE = 2;

const double BALL_RADIUS = .029;
const double POCKET_RADIUS = .05;
const double WIDTH = 3;
const double HEIGHT = 1.5;
const double FRICTION = 0.1;
const double RAIL_RES = 0.75;
const double BALL_RES = 0.95;

const double MAX_ANIMATION_LENGTH = .5;

//Structure that holds a ball object
struct ball {
  vector pos;
  vector vel;
  int id;
  int inPocket; // 0 if not in pocket

  ball(vector _pos, int _id) {
    pos = _pos;
    vel = vector();
    id = _id;
    inPocket = -1;
  }

  void run(double dt) {
    pos += vel * dt;
    double speed = abs(vel);
    double newSpeed = speed - FRICTION * dt;
    if (newSpeed < 0) {
      newSpeed = 0;
    }
    if (speed != 0) {
      vel *= newSpeed / speed;
    }
  }
};

//Snapshot of the state of the table
struct state {
  double time;
  int numballs;
  ball *balls;
};

//Take the dot product of two vectors
double dot(vector a, vector b) {
  return a.X * b.X + a.Y * b.Y;
}

//Lulz take the square of a number
inline double square(double x) {
  return x * x;
}

//Returns the projection of vector a onto vector b
vector proj(vector a, vector b) {
  return dot(a, b) / square(abs(b)) * b;
}

//Uses law of cosines to calculate when ball a travels within (BALL_RADIUS+radius2) of ball b
//Used within collideBalls and collidePocket
double collideHelper(ball a, ball b, double dt, double radius2) {
  double r = BALL_RADIUS;

  vector dp = a.pos - b.pos, dv = a.vel - b.vel;
  double cos = -1 * dot(dp, dv) / abs(dp * dv);
  double diffD = abs(dp), diffV = abs(dv);
  double disc = square(2 * diffD * cos) - 4 * (square(diffD) - 4 * r * radius2);
  if (disc < 0) {
    return -1;
  }
  double ans = (2 * diffD * cos - sqrt(disc)) / (2 * diffV);
  if (ans <= 0) return -1;
  else return ans;
}

//Returns at what time the two balls collide
double collideBalls(ball a, ball b, double dt) {
  return collideHelper(a, b, dt, BALL_RADIUS);
}

//Determines whether, given some x coordinate, there will be a wall at y=0 or y=height
bool isValidWallWidth(double x){
    double corner = CORNER_WALL_MISSING, side = SIDE_WALL_MISSING, width = WIDTH;
    return ( (corner < x && x < width / 2 - side) || (width / 2 + side < x && x < width - corner) );
}

//Determines whether, given some y coordinate, there will be a wall at x=0 or x=width
bool isValidWallHeight(double y){
    double corner = CORNER_WALL_MISSING, side = SIDE_WALL_MISSING, height = HEIGHT;
    return ( (corner < y && y < height / 2 - side) || (height / 2 + side < y && y < height - corner) );
}

//Returns at what time the ball collides with the wall
double collideWall(ball cur, int wallId, double dt) {
  double x = cur.pos.X, y = cur.pos.Y;
  double vx = cur.vel.X, vy = cur.vel.Y;
  double nx = x + vx * dt, ny = y + vy * dt;
  double r = BALL_RADIUS;
  double width = WIDTH, height = HEIGHT;

  if (wallId == 0) { // (width, 0) -- (0, 0)
    if (ny < r) {
      double t = (r - y) / vy;
      double xAtT = x + vx * t;
      if(isValidWallWidth(xAtT)) { return t;}
    }
  } else if (wallId == 1) { // (width, height) -- (width, 0)
    if (nx > width - r) {
      double t = (width - r - x) / vx;
      double yAtT = y + vy * t;
      if( isValidWallHeight(yAtT)) { return t;}
    }
  } else if (wallId == 2) { // (0, height) -- (width, height)
    if (ny  > height - r) {
      double t = (height - r - y) / vy;
      double xAtT = x + vx * t;
      if(isValidWallWidth(xAtT)) { return t;}
    }
  } else if (wallId == 3) { // (0, 0) -- (0, height)
    if (nx < r) {
      double t = (r - x) / vx;
      double yAtT = y + vy * t;
      if( isValidWallHeight(yAtT)) { return t;}
    }
  }
  return -1;
}

//Returns at what time the ball colliddes with the pocket (ie gets sunk)
//Pockets are labeled as such - top row is 012, bottom row is 345
double collidePocket(ball cur, int pockID, double dt) {
  double width = WIDTH, height = HEIGHT, corner = CORNER_WITHIN_WALL, side = SIDE_WITHIN_WALL;
  double x = 0,y = 0;
  if(pockID == 0){
    x = -corner, y = -corner;
  } else if (pockID == 1){
    x = width / 2, y = -side;
  } else if (pockID == 2){
    x = width + corner, y = -corner;
  } else if (pockID == 3){
    x = -corner, y = height + corner;
  } else if (pockID == 4){
    x = width / 2, y = height + side;
  } else if(pockID == 5){
    x = width + corner, y = height + corner;
  }
  ball pocketBall = ball(vector(x, y), pockID);
  return collideHelper(cur, pocketBall, dt, POCKET_RADIUS - BALL_RADIUS);
}

//Used to help determine corner pocket walls
double collideCornerPocketWallHelper0(double x, double y, vector v, double dt){
  double corner = CORNER_WALL_MISSING, slope = CORNER_SLOPE;
  double newX = x + dt * v.X, newY = y + dt * v.Y;
  vector norm = v - proj(v, vector(1, slope));
  if(abs(proj(vector(-newX,corner-newY), norm)) < BALL_RADIUS && newX < 0){
    return (abs(proj(vector(corner-x,-y), norm) - BALL_RADIUS) / abs(norm));
  } return -1;
}
double collideCornerPocketWallHelper1(double x, double y, vector v, double dt){
  double corner = CORNER_WALL_MISSING, slope = CORNER_SLOPE;
  double newX = x + dt * v.X, newY = y + dt * v.Y;
  vector norm = v - proj(v, vector(1, slope));
  if(abs(proj(vector(corner-newX,-newY), norm)) < BALL_RADIUS && newY < 0){
    return (abs(proj(vector(corner-x,-y), norm) - BALL_RADIUS) / abs(norm));
  } return -1;
}
double collideCornerPocketWallHelper2(double x, double y, vector v, double dt){
  double side = SIDE_WALL_MISSING, slope = -SIDE_SLOPE;
  double newX = x + dt * v.X, newY = y + dt * v.Y;
  vector norm = v - proj(v, vector(1, slope));
  if(abs(proj(vector(-side-newX,-newY), norm)) < BALL_RADIUS && newY < 0){
    return (abs(proj(vector(-side-x,-y), norm) - BALL_RADIUS) / abs(norm));
  } return -1;
}
double collideCornerPocketWallHelper3(double x, double y, vector v, double dt){
  double side = SIDE_WALL_MISSING, slope = SIDE_SLOPE;
  double newX = x + dt * v.X, newY = y + dt * v.Y;
  vector norm = v - proj(v, vector(1, slope));
  if(abs(proj(vector(side-newX,-newY), norm)) < BALL_RADIUS && newY < 0){
    return (abs(proj(vector(side-x,-y), norm) - BALL_RADIUS) / abs(norm));
  } return -1;
}
//Returns at what time the ball collides with the pocket walls (ie the tiny ones right next to the pockets)
//Pocket walls are labeled as such - top row is 012345, bottom row is 67891011
//For bottom pocket walls, REFLECT everything vertically to create parallel cases
//For right corner pocket walls, REFLECT everything horizontally to create parallel cases
double collidePocketWall(ball cur, int pockWallID, double dt) {
    double width = WIDTH, height = HEIGHT;
    double x = cur.pos.X, y = cur.pos.Y; //ball location relative to the center of the pocket
    double vx = cur.vel.X, vy = cur.vel.Y;
    double slope; 
    if(pockWallID == 0){
      return collideCornerPocketWallHelper0(cur.pos.X, cur.pos.Y, cur.vel, dt);
    } else if(pockWallID == 1){
      return collideCornerPocketWallHelper1(cur.pos.X, cur.pos.Y, cur.vel, dt);
    }else if(pockWallID == 2){
      return collideCornerPocketWallHelper2(cur.pos.X - width / 2, cur.pos.Y, cur.vel, dt);
    }else if(pockWallID == 3){
      return collideCornerPocketWallHelper3(cur.pos.X - width / 2, cur.pos.Y, cur.vel, dt);
    }else if(pockWallID == 4){
      return collideCornerPocketWallHelper1(-(cur.pos.X - width), cur.pos.Y, vector(-cur.vel.X, cur.vel.Y), dt);
    }else if(pockWallID == 5){
      return collideCornerPocketWallHelper1(-(cur.pos.X - width), cur.pos.Y, vector(-cur.vel.X, cur.vel.Y), dt);
    }else if(pockWallID == 6){
      return collideCornerPocketWallHelper1(cur.pos.X, -(cur.pos.Y - height), vector(cur.vel.X, -cur.vel.Y), dt);
    }else if(pockWallID == 7){
      return collideCornerPocketWallHelper1(cur.pos.X, -(cur.pos.Y - height), vector(cur.vel.X, -cur.vel.Y), dt);
    }else if(pockWallID == 8){
      return collideCornerPocketWallHelper1(cur.pos.X - width / 2, -(cur.pos.Y - height), vector(cur.vel.X, -cur.vel.Y), dt);
    }else if(pockWallID == 9){
      return collideCornerPocketWallHelper1(cur.pos.X - width / 2, -(cur.pos.Y - height), vector(cur.vel.X, -cur.vel.Y), dt);
    }else if(pockWallID == 10){
      return collideCornerPocketWallHelper1(-(cur.pos.X - width), -(cur.pos.Y - height), vector(-cur.vel.X, -cur.vel.Y), dt);
    }else if(pockWallID == 11){
      return collideCornerPocketWallHelper1(-(cur.pos.X - width), -(cur.pos.Y - height), vector(-cur.vel.X, -cur.vel.Y), dt);
    }
}

//Adjusts velocities when two balls collide
void handleCollide(ball &a, ball &b) {
  vector dd = b.pos - a.pos;
  vector va = a.vel, vb = b.vel;
  vector tang = vector(dd.Y, -dd.X);
  vector vat = proj(va, tang), vbt = proj(vb, tang);
  a.vel = vat + BALL_RES * (vb - vbt);
  b.vel = vbt + BALL_RES * (va - vat);
}

//Adjusts velocities when a ball collides with the wall
void handleCollideWall(ball &a, int wallId) {
  if (wallId == 0 || wallId == 2) {
    a.vel = vector(a.vel.X, -RAIL_RES * a.vel.Y);
  } else if (wallId == 1 || wallId == 3) {
    a.vel = vector(-a.vel.X, RAIL_RES * a.vel.Y);
  }
}

//Adjusts velocities when a ball goes in a pocket
void handleCollidePocket(ball &a, int pocketID) {
  a.inPocket = pocketID;
  //PROBABLY SHOULD DO SOMETHING HERE
}

//Adjusts velocities when a ball collides with a pocketwall
void handleCollidePocketWall(ball &a, int pocketWallID) {
  double slope = 0;
    if(pocketWallID == 0 || pocketWallID == 1 || pocketWallID == 10 || pocketWallID == 11){
      slope = CORNER_SLOPE;
    } else if(pocketWallID == 4 || pocketWallID == 5 || pocketWallID == 6 || pocketWallID == 7){
      slope = -CORNER_SLOPE;
    } else if(pocketWallID == 2 || pocketWallID == 9){
      slope = -SIDE_SLOPE;
    } else if(pocketWallID == 3 || pocketWallID == 8){
      slope = SIDE_SLOPE;
    }
    vector tan = proj(a.vel, vector(1, slope));
    a.vel = tan - RAIL_RES * (a.vel - tan);
}

bool onTable(state cur, int ballID){
  return cur.balls[ballID].inPocket == -1;
}

//From one state, calculates the next step
state next(state cur) {
  double next_default_time = ceil(cur.time / DEFAULT_TIME_STEP +.0001) * DEFAULT_TIME_STEP;
  double dt = next_default_time - cur.time;
  state nxt = cur;
  int n = cur.numballs;

  int collidei = -1, collidej = -1;
  int collideType = -1; // -1: no, 0: balls, 1: pocket, 2: wall
  for(int i = 0; i < n; ++i) {
    if(onTable(cur, i)){
      for(int j = 1; j < n; ++j) {
        if(onTable(cur, j)){
          double t = collideBalls(cur.balls[i], cur.balls[j], dt);
          if (t != -1 && 0 < t && t < dt) {
            collidei = i;
            collidej = j;
            collideType = 0;
            dt = t;
          }
        }
      }
    }
  }

  for(int i = 0; i < n; ++i) {
    if(onTable(cur, i)){
      for(int j = 0; j < 6; ++j) {
        double t = collidePocket(cur.balls[i], j, dt);
        if(t != -1 && 0 < t && t < dt) {
          collidei = i;
          collidej = j;
          collideType = 1;
          dt = t;
        }
      }
    }
  }

  for(int i = 0; i < n; ++i) {
    if(onTable(cur,i)){
      for(int j = 0; j < 4; ++j) {
        double t = collideWall(cur.balls[i], j, dt);
        if (t != -1 && 0 < t && t < dt) {
          collidei = i;
          collidej = j;
          collideType = 2;
          dt = t;
        }
      }
    }
  }
  
  for(int i = 0; i < n; ++i) {
    if(onTable(cur,i)){
      for(int j = 0; j < 12; ++j) {
        double t = collidePocketWall(cur.balls[i], j, dt);
        if (t != -1 && 0 < t && t < dt) {
          collidei = i;
          collidej = j;
          collideType = 3;
          dt = t;
        }
      }
    }
  }

  for(int i = 0; i < n; ++i) {
    if(onTable(cur, i)){
      cur.balls[i].run(dt);
    }
  }

  if (collideType == 0) {
    printf("Collision type 0, dt = %.02lf, i: %d, j: %d\n", dt, collidei, collidej);
     // handle collision between collidei, collidej
    handleCollide(cur.balls[collidei], cur.balls[collidej]);
  } else if(collideType == 1) {
    printf("Collision type 1, dt = %.02lf, i: %d j: %d\n", dt, collidei, collidej);
    handleCollidePocket(cur.balls[collidei], collidej);
  }
  else if (collideType == 2) {
    printf("Collision type 2, dt = %.02lf, i: %d j: %d\n", dt, collidei, collidej);
    handleCollideWall(cur.balls[collidei], collidej);
    // handle wall collision between collidei with wall collidej
  }
  else if (collideType == 3) {
    printf("Collision type 3, dt = %.02lf, i: %d j: %d\n", dt, collidei, collidej);
    handleCollidePocketWall(cur.balls[collidei], collidej);
    // handle wall collision between collidei with pocket wall collidej
  }

  nxt.time += dt;
  return nxt;
}

//Returns random num from -.5 to .5
double rnd() {
  return (rand() % 100 - 50) / 100.;
}

//Default state for our simulation
state makeDefaultState() {
  state s;
  s.time = 0;
  s.numballs = 1;
  s.balls = (ball *)malloc(s.numballs*sizeof(ball));
  for(int i = 0; i < s.numballs; ++i) {
    s.balls[i] = ball(vector(.04, .1), i);
    s.balls[i].vel = vector(0, -1);
  }
  return s;
}

//Displays stuff
void disp(state cur) {
  printf("TIME=%f\n", cur.time);
  for(int i = 0; i < cur.numballs; ++i) {
    ball &b = cur.balls[i];
    printf("BALL(%d): p=(%.2lf, %.2lf) v=(%.2lf, %.4lf)\n", b.id, b.pos.X, b.pos.Y, b.vel.X, b.vel.Y);
  }
  printf("==============\n");
}

//Returns if x is close enough to an integer
bool isInteger(double x){
  double epsilon = .0001;
  double fracPart = x - (int) x;
  return ((fracPart < epsilon) || (fracPart > 1 - epsilon));
}

//Main method for the API
//Returns an array of all the states
state* allStates(state beginning) {
  int numFrames = MAX_ANIMATION_LENGTH / DEFAULT_TIME_STEP + 1;
  state *stateList = new state[numFrames];
  state cur = beginning;
  for(int i = 0; i < numFrames; ++i) {
    while(!isInteger(cur.time / DEFAULT_TIME_STEP)){
      cur = next(cur);
    }
    stateList[i] = cur;
    stateList[i].balls = (ball*) malloc(sizeof(ball) * cur.numballs);
    for(int j = 0; j < cur.numballs; ++j) {
      stateList[i].balls[j] = cur.balls[j];
    }
    
    cur = next(cur);
  }
  free(cur.balls);
  return stateList;
}

//Whooo starts simulation
int main() {
  state cur;
  cur = makeDefaultState();
  printf("STARTING SIMULATION\n");
  for(int i = 0; i < 10; ++i) {
    cur = next(cur);
    disp(cur);
  }
  /*state *stateList = allStates(cur);
  *for(int i = 0; i < 10; ++i) {
  * disp(stateList[i]);
  }*/
}