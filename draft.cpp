#include "game_logic.h"

double calculate_distance(double x1, double y1, double x2, double y2){
    double dx = x1 - x2;
    double dy = y1 - y2;
    return sqrt(dx * dx + dy * dy);
}

int find_main_tower_index(game_session &session){
    for(int i = 0; i < length(session.towers); i++){
        if(session.towers[i].is_main_tower)
            return i;
    }
    return -1;
}

bullet create_bullet(double start_x, double start_y, double target_monster_x, double target_monster_y, double damage){
    bullet b;
    b.x = start_x;
    b.y = start_y;
    b.target_x = target_monster_x;
    b.target_y = target_monster_y;
    b.damage = damage;
    b.active = true;
    b.speed = GAME_CONFIG.BULLET.SPEED;

    double dx = b.target_x - b.x;
    double dy = b.target_y - b.y;
    double distance = calculate_distance(b.x, b.y, b.target_x, b.target_y);

    if(distance != 0){
        b.direction_x = dx / distance;
        b.direction_y = dy / distance;
    }
    else{
        b.direction_x = 0;
        b.direction_y = 0;
    }

    return b;
}

tower create_main_tower(){
    tower main_tower;
    main_tower.x = GAME_CONFIG.SCREEN_WIDTH / 2.0;
    main_tower.y = GAME_CONFIG.SCREEN_HEIGHT / 2.0;
    main_tower.is_main_tower = true;
    main_tower.damage = GAME_CONFIG.MAIN_TOWER.DAMAGE;
    main_tower.health = GAME_CONFIG.MAIN_TOWER.HEALTH;
    main_tower.max_health = GAME_CONFIG.MAIN_TOWER.MAX_HEALTH;
    return main_tower;
}

tower create_sub_tower(double x, double y){
    tower sub_tower;
    sub_tower.x = x;
    sub_tower.y = y;
    sub_tower.damage = GAME_CONFIG.SUB_TOWER.DAMAGE;
    sub_tower.health = GAME_CONFIG.SUB_TOWER.HEALTH;
    sub_tower.max_health = GAME_CONFIG.SUB_TOWER.MAX_HEALTH;
    return sub_tower;
}

void init_grid(game_session &session){
    for(int r = 0; r < GAME_CONFIG.MAP_DIMENSION; r++){
        for(int c = 0; c < GAME_CONFIG.MAP_DIMENSION; c++){
            session.grid[r][c].single_tile = EMPTY_TILE;
        }
    }

    for(int r = GAME_CONFIG.MAIN_TOWER_GRID_START; r < GAME_CONFIG.MAIN_TOWER_GRID_END; r++){
        for(int c = GAME_CONFIG.MAIN_TOWER_GRID_START; c < GAME_CONFIG.MAIN_TOWER_GRID_END; c++){
            session.grid[r][c].single_tile = MAIN_TOWER_TILE;
        }
    }
}

void init_game_session(game_session &session){
    session.currency = 0;
    session.wave_number = 1;
    session.is_game_over = false;
    session.monster_left_to_spawn = GAME_CONFIG.SESSION.INITIAL_MONSTER_TO_SPAWN;

    session.bullets = {};
    session.monsters = {};
    session.towers = {};

    init_grid(session);
    tower main_tower = create_main_tower();
    add(session.towers, main_tower);
}

void set_spawn_position(monster &m, direction side){
    switch(side){
        case UP:
        { 
            m.x = rnd(GAME_CONFIG.MAP_LEFT, GAME_CONFIG.MAP_RIGHT);
            m.y = GAME_CONFIG.MAP_TOP;
            break;
        }    
        case RIGHT:
        {
            m.x = GAME_CONFIG.MAP_RIGHT;
            m.y = rnd(GAME_CONFIG.MAP_TOP, GAME_CONFIG.MAP_BOTTOM);
            break;
        }
        case DOWN:
        {
            m.x = rnd(GAME_CONFIG.MAP_LEFT, GAME_CONFIG.MAP_RIGHT);
            m.y = GAME_CONFIG.MAP_BOTTOM;
            break;
        }
        case LEFT:
        {
            m.x = GAME_CONFIG.MAP_LEFT;
            m.y = rnd(GAME_CONFIG.MAP_TOP, GAME_CONFIG.MAP_BOTTOM);
            break;
        }
        default:
            break;
    }
}

void spawn_monster(game_session &session){
    monster m;
    
    direction side = (direction)(rnd(0, DIRECTION_COUNT - 1));
    set_spawn_position(m, side);

    double hp_multiplier = session.wave_number * 2.0;
    m.health = hp_multiplier * GAME_CONFIG.MONSTER.HEALTH;
    m.max_health = m.health;
    m.alive = true;
    m.attack_cooldown = 0;
    m.speed = GAME_CONFIG.MONSTER.SPEED;
    m.damage = GAME_CONFIG.MONSTER.DAMAGE;

    add(session.monsters, m);
}

void update_wave(game_session &session){
    if(session.monster_left_to_spawn <= 0){
        session.wave_number++;
        session.monster_left_to_spawn = GAME_CONFIG.SESSION.INITIAL_MONSTER_TO_SPAWN * session.wave_number;
    }
}

void update_monster(game_session &session){
    for(int i = 0; i < length(session.monsters); i++){
        if(!session.monsters[i].alive) continue;
        int nearest_sub_tower_index = -1;
        double min_distance = GAME_CONFIG.SUB_TOWER.COLLISION_RANGE;
        for(int j = 0; j < length(session.towers); j++){
            if(session.towers[j].is_main_tower) continue;

            double distance = calculate_distance(session.monsters[i].x, session.monsters[i].y, session.towers[j].x, session.towers[j].y);
            
            if(distance <= min_distance){
                nearest_sub_tower_index = j;
                min_distance = distance;
            }
        }

        if(nearest_sub_tower_index != -1){
            apply_monster_attack(session.monsters[i], session.towers[nearest_sub_tower_index]);

            if(session.towers[nearest_sub_tower_index].health <= 0){
                remove_at(session.towers, nearest_sub_tower_index);
            }
        }
        else{
            int main_tower_index = find_main_tower_index(session);
            if(main_tower_index == -1) continue;
            double distance = calculate_distance(session.monsters[i].x, session.monsters[i].y, session.towers[main_tower_index].x, session.towers[main_tower_index].y);
            
            if(distance <= GAME_CONFIG.MAIN_TOWER.COLLISION_RANGE){
                apply_monster_attack(session.monsters[i], session.towers[main_tower_index]);
            }
            else{
                double dx = session.towers[main_tower_index].x - session.monsters[i].x;
                double dy = session.towers[main_tower_index].y - session.monsters[i].y;
                if(distance != 0){
                    double direction_x = dx / distance;
                    double direction_y = dy / distance;
                    session.monsters[i].x += direction_x * GAME_CONFIG.MONSTER.SPEED;
                    session.monsters[i].y += direction_y * GAME_CONFIG.MONSTER.SPEED;
                }
            }
        }
    }
}

bool apply_monster_attack(monster &m, tower &target_tower){
    if(m.attack_cooldown <= 0){
        target_tower.health -= GAME_CONFIG.MONSTER.DAMAGE;
        m.attack_cooldown = GAME_CONFIG.MONSTER.ATTACK_COOLDOWN;
        return true; //maybe no need
    }
    else{
        m.attack_cooldown--;
        return false; //maybe no need
    }
}

bool is_game_over(game_session &session){
    for(int i = 0; i < length(session.towers); i++){
        if(session.towers[i].is_main_tower && session.towers[i].health <= 0){
            return true;
        }
    }
    return false;
}

void update_tower(game_session &session){
    
    for(int i = 0; i < length(session.towers); i++){
        if(session.towers[i].cooldown > 0){
            session.towers[i].cooldown--;
            continue;
        }
    
        int target_monster_index = -1;
        double min_distance;

        if(session.towers[i].is_main_tower){
            min_distance = GAME_CONFIG.MAIN_TOWER.RANGE;
        }
        else{
            min_distance = GAME_CONFIG.SUB_TOWER.RANGE;
        }

        for(int j = 0; j < length(session.monsters); j++){
            if(!session.monsters[j].alive) continue;
            
            double distance = calculate_distance(session.towers[i].x, session.towers[i].y, session.monsters[j].x, session.monsters[j].y);
        
            if(distance <= min_distance){
                target_monster_index = j;
                min_distance = distance;
            }
        }


        if(target_monster_index != -1){
            bullet b = create_bullet(session.towers[i].x, session.towers[i].y, session.monsters[target_monster_index].x, session.monsters[target_monster_index].y, session.towers[i].damage);
            add(session.bullets, b);
            session.towers[i].cooldown = GAME_CONFIG.SUB_TOWER.ATTACK_COOLDOWN;
        }


    }
}

bool bullet_collides_with_monster(const bullet &b, const monster &m){
    double distance = calculate_distance(b.x, b.y, m.x, m.y);
    return (distance <= GAME_CONFIG.MONSTER.RADIUS);
}

void update_bullet(game_session &session){
    for(int i = 0; i < length(session.bullets); i++){
        if(!session.bullets[i].active) continue;
        
        session.bullets[i].x += session.bullets[i].direction_x * session.bullets[i].speed;
        session.bullets[i].y += session.bullets[i].direction_y * session.bullets[i].speed;

        if(session.bullets[i].x > GAME_CONFIG.MAP_RIGHT || session.bullets[i].x < GAME_CONFIG.MAP_LEFT || session.bullets[i].y > GAME_CONFIG.MAP_BOTTOM || session.bullets[i].y < GAME_CONFIG.MAP_TOP){
            session.bullets[i].active = false;
            continue;
        }

        for(int j = 0; j < length(session.monsters); j++){
            if(!session.monsters[j].alive) continue;

            if(bullet_collides_with_monster(session.bullets[i], session.monsters[j])){
                session.monsters[j].health -= session.bullets[i].damage;
                session.bullets[i].active = false;

                if(session.monsters[j].health <= 0){
                    session.monsters[j].alive = false;
                    session.currency += GAME_CONFIG.MONSTER.KILL_REWARD;
                }

                break;
            }
        }
    }
}

void remove_dead_monster(game_session &session){
    for(int i = length(session.monsters) - 1; i >= 0; i--){
        if(!session.monsters[i].alive){
            remove_at(session.monsters, i);
        }
    }
}

void remove_inactive_bullet(game_session &session){
    for(int i = length(session.bullets) - 1; i >= 0; i--){
        if(!session.bullets[i].active){
            remove_at(session.bullets, i);
        }
    }
}

void apply_tower_upgrade(tower &t){
    t.level++;
    t.damage += GAME_CONFIG.TOWER_UPGRADE.DAMAGE_INCREMENT;
    t.range += GAME_CONFIG.TOWER_UPGRADE.RANGE_INCREMENT;
    t.health += GAME_CONFIG.TOWER_UPGRADE.HEALTH_INCREMENT;
    t.max_health += GAME_CONFIG.TOWER_UPGRADE.HEALTH_INCREMENT;
}

bool upgrade_tower(game_session &session, int tower_index){
    int upgrade_cost = GAME_CONFIG.TOWER_UPGRADE.BASE_UPGRADE_COST * session.towers[tower_index].level; 
    if(session.towers[tower_index].is_main_tower || session.towers[tower_index].level >= GAME_CONFIG.SUB_TOWER.MAX_LEVEL || session.currency < upgrade_cost) return false;
    session.currency -= upgrade_cost;
    apply_tower_upgrade(session.towers[tower_index]);
    return true;
}

bool build_tower(game_session &session, int row, int col){
    if(session.currency < GAME_CONFIG.SUB_TOWER.COST) return false;
    session.currency -= GAME_CONFIG.SUB_TOWER.COST;
    double x = GAME_CONFIG.OFFSET_X + col * GAME_CONFIG.TILE_SIZE + GAME_CONFIG.TILE_SIZE / 2.0;
    double y = GAME_CONFIG.OFFSET_Y + row * GAME_CONFIG.TILE_SIZE + GAME_CONFIG.TILE_SIZE / 2.0;
    tower t = create_sub_tower(x, y);
    add(session.towers, t);
    session.grid[row][col].single_tile = SUB_TOWER_TILE;
    session.grid[row][col].tower_index = length(session.towers) - 1;

    return true;
}

void handle_input(game_session &session){
    if(mouse_clicked(LEFT_BUTTON)){
        double mousex = mouse_x();
        double mousey = mouse_y();

        double adjusted_x = mousex - GAME_CONFIG.OFFSET_X;
        double adjusted_y = mousey - GAME_CONFIG.OFFSET_Y;

        if(adjusted_x < 0 || adjusted_x >= GAME_CONFIG.MAP_DIMENSION * GAME_CONFIG.TILE_SIZE || adjusted_y < 0 || adjusted_y >= GAME_CONFIG.MAP_DIMENSION * GAME_CONFIG.TILE_SIZE){
            return;
        }

        int col = adjusted_x / GAME_CONFIG.TILE_SIZE;
        int row = adjusted_y / GAME_CONFIG.TILE_SIZE;

        if(session.grid[row][col].single_tile == EMPTY_TILE){
            build_tower(session, row, col);
        }
        else if(session.grid[row][col].single_tile == SUB_TOWER_TILE){
            upgrade_tower(session, session.grid[row][col].tower_index);
        }


    }
}