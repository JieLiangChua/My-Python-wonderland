#include "game_logic.h"

// Function to calculate the Euclidean distance between two points (x1, y1) and (x2, y2)
double calculate_distance(double x1, double y1, double x2, double y2){
    double dx = x1 - x2;
    double dy = y1 - y2;
    return sqrt(dx * dx + dy * dy); // Return the Euclidean distance
}

// Procedure to initialize a new game session with default values and set up the grid and main tower
void init_game_session(game_session &session){
    session.currency = GAME_CONFIG.SESSION.INITIAL_CURRENCY;
    session.wave_number = GAME_CONFIG.SESSION.INITIAL_WAVE_NUMBER;

    session.monster_left_to_spawn = session.wave_number * GAME_CONFIG.SESSION.INITIAL_MONSTER_LEFT_TO_SPAWN; // Initiate the number of monster to spawn in the first wave
    session.spawn_timer = 0.0; // Initialize the spawn timer to count the time between monster spawns

    session.towers = {};
    session.monsters = {};
    session.bullets = {};

    init_grid(session); // Call the function to initialise the grid with default tile types and set up the main tower area
    add(session.towers, create_main_tower()); // Call the function to create and add the main tower to the towers array
}

// Procedure to initialize the grid with default tile types and set up the main tower area
void init_grid(game_session &session){
    // Initialize the grid with default tile types and occupancy status
    for(int r = 0; r < GAME_CONFIG.MAP_DIMENSION; r++){
        for(int c = 0; c < GAME_CONFIG.MAP_DIMENSION; c++){
            session.grid[r][c].single_tile = EMPTY_TILE; // Set all tiles to EMPTY_TILE initially
        }
    }

    // Initialize a 2x2 area in the center of the grid as the main tower ((4,4), (4,5), (5,4), (5,5))
    for(int r = GAME_CONFIG.MAIN_TOWER_GRID_START; r <= GAME_CONFIG.MAIN_TOWER_GRID_END; r++){
        for(int c = GAME_CONFIG.MAIN_TOWER_GRID_START; c <= GAME_CONFIG.MAIN_TOWER_GRID_END; c++){
            session.grid[r][c].single_tile = MAIN_TOWER_TILE; // Set the tiles in the main tower area to MAIN_TOWER_TILE
        }
    }
}

// Function to create and initialise the main tower with its properties
tower create_main_tower(){
    tower main_tower;
    main_tower.x = GAME_CONFIG.SCREEN_WIDTH / 2.0;
    main_tower.y = GAME_CONFIG.SCREEN_HEIGHT / 2.0;
    main_tower.damage = GAME_CONFIG.MAIN_TOWER.DAMAGE;
    main_tower.range = GAME_CONFIG.MAIN_TOWER.RANGE;
    main_tower.tower_health = GAME_CONFIG.MAIN_TOWER.HEALTH;
    main_tower.max_tower_health = GAME_CONFIG.MAIN_TOWER.MAX_HEALTH;
    main_tower.is_main_tower = true;
    return main_tower;
}

// Function to apply damage from a monster to a tower, considering the monster's attack cooldown
bool apply_monster_attack(monster &m, tower &target_tower){
    if(m.attack_cooldown <= 0){ // Check if the monster can attack
        target_tower.tower_health -= GAME_CONFIG.MONSTER.DAMAGE; // Apply damage to the target tower
        m.attack_cooldown = GAME_CONFIG.MONSTER.ATTACK_COOLDOWN; // Reset the monster's attack cooldown after a successful attack
        return true; // Attack was successful
    }
    else{
        m.attack_cooldown--; // Decrease the monster's attack cooldown timer
        return false; // Still in cooldown, attack not applied
    }
}

// Procedure to set the spawn position of a monster based on the specified direction (UP, RIGHT, DOWN, LEFT)
void set_spawn_position(monster &new_monster, direction side){
    switch(side){
        case UP:
        { 
            new_monster.x = rnd(GAME_CONFIG.MAP_LEFT, GAME_CONFIG.MAP_RIGHT); // Random x position along the top edge
            new_monster.y = GAME_CONFIG.MAP_TOP; // y position at the top edge
            break;
        }    
        case RIGHT:
        {
            new_monster.x = GAME_CONFIG.MAP_RIGHT;
            new_monster.y = rnd(GAME_CONFIG.MAP_TOP, GAME_CONFIG.MAP_BOTTOM); // Random y position along the right edge
            break;
        }
        case DOWN:
        {
            new_monster.x = rnd(GAME_CONFIG.MAP_LEFT, GAME_CONFIG.MAP_RIGHT);
            new_monster.y = GAME_CONFIG.MAP_BOTTOM; // y position at the bottom edge
            break;
        }
        case LEFT:
        {
            new_monster.x = GAME_CONFIG.MAP_LEFT;
            new_monster.y = rnd(GAME_CONFIG.MAP_TOP, GAME_CONFIG.MAP_BOTTOM); // Random y position along the left edge
            break;
        }
        default:
            break;
    }
}

// Procedure to spawn a new monster
void spawn_monster(game_session &session){
    monster new_monster;

    double hp_multiplier = 1.0 + (session.wave_number - 1) * GAME_CONFIG.MONSTER.HP_GROWTH_PER_WAVE; // Increase health with each wave
    new_monster.max_monster_health = GAME_CONFIG.MONSTER.MAX_HEALTH * hp_multiplier; // Set the maximum health of the monster based on the wave number
    new_monster.monster_health = new_monster.max_monster_health; // Set the current health of the monster to its maximum health

    new_monster.speed = GAME_CONFIG.MONSTER.SPEED;
    new_monster.alive = true;
    new_monster.attack_cooldown = 0;
    new_monster.max_attack_cooldown = GAME_CONFIG.MONSTER.ATTACK_COOLDOWN;

    direction side = (direction)(rnd(0, DIRECTION_COUNT - 1)); // Randomly select a side (UP, RIGHT, DOWN, LEFT) for the monster to spawn from
    set_spawn_position(new_monster, side); // Set the spawn position of the monster based on the selected side
    
    add(session.monsters, new_monster); // Add the spawned monster to the monsters array
}

// Procedure to update the monsters in the game session, moving them towards the main tower and checking for collisions
void update_monster(game_session &session){
    int main_tower_idx = -1; // Find the index of the main tower in the towers array
    // Loop through the towers to find the main tower
    for(int k = 0; k < length(session.towers); k++) {
        if (session.towers[k].is_main_tower) {
            main_tower_idx = k;
            break;
        }
    }

    if(main_tower_idx == -1) return; // If no main tower is found, exit the function (should not happen in a valid game session)

    for(int i = 0; i < length(session.monsters); i++){
        monster &m = session.monsters[i];
        if(!m.alive) continue; // Skip dead monsters
        
        tower &main_tower = session.towers[main_tower_idx]; // Get the main tower from the towers array

        int target_sub_tower_idx = -1;
        double min_sub_distance = GAME_CONFIG.SUB_TOWER.COLLISION_RANGE; // Initialize the minimum distance to the sub-tower collision range

        for(int k = 0; k < length(session.towers); k++){
            const tower &t = session.towers[k];
            if(!t.is_main_tower){
                double hypotenuse_distance = calculate_distance(m.x, m.y, t.x, t.y); // Calculate the distance between the monster and the sub-tower
                if(hypotenuse_distance <= min_sub_distance){
                    min_sub_distance = hypotenuse_distance;
                    target_sub_tower_idx = k;
                }
            }
        }

        // Branch A: There is a sub-tower within the monster's collision range
        if(target_sub_tower_idx != -1){
            tower &sub_tower = session.towers[target_sub_tower_idx];
            apply_monster_attack(m, sub_tower); // Apply damage to the sub-tower if the monster is within range
            
            if(sub_tower.tower_health <= 0){
                int col = (sub_tower.x - GAME_CONFIG.OFFSET_X) / GAME_CONFIG.TILE_SIZE; // Calculate the column index of the destroyed sub-tower in the grid
                int row = (sub_tower.y - GAME_CONFIG.OFFSET_Y) / GAME_CONFIG.TILE_SIZE;
                session.grid[row][col].single_tile = EMPTY_TILE;
                session.grid[row][col].tower_index = -1; // Reset the tower index in the grid for the destroyed sub-tower

                remove_at(session.towers, target_sub_tower_idx);
                write_line("[DEBUG] Sub tower destroyed by monster at (" + to_string(row) + ", " + to_string(col) + ")");
            }
        }

        // Branch B: There is no sub-tower within the monster's collision range, so the monster moves towards the main tower
        else{
            double dx = main_tower.x - m.x;
            double dy = main_tower.y - m.y;
            double hypotenuse_distance = calculate_distance(m.x, m.y, main_tower.x, main_tower.y); // Calculate the distance between the monster and the main tower

            if(hypotenuse_distance <= GAME_CONFIG.MAIN_TOWER.COLLISION_RANGE){
                if(apply_monster_attack(m, main_tower)){ // Apply damage to the main tower if the monster is within range
                    write_line("[DEBUG] Monster collided with main tower! Main tower health: " + to_string(main_tower.tower_health));
                }
            }
            else if(hypotenuse_distance > 0){
                double move_x = (dx / hypotenuse_distance) * m.speed; // Calculate the x movement based on speed and direction
                double move_y = (dy / hypotenuse_distance) * m.speed; // Calculate the y movement based on speed and direction
                m.x += move_x; // Update the monster's x position
                m.y += move_y; // Update the monster's y position
            }
        }
    }
}
    
// Procedure to update the towers in the game session, checking for monsters within range and applying damage
void update_tower(game_session &session){
    for(int i = 0; i < length(session.towers); i++){
        tower &t = session.towers[i]; // Loop through each tower
        
        if(t.cooldown > 0){
            t.cooldown--; // Decrease the cooldown timer for the tower
            continue; // Skip to the next tower if this one is still cooling down
        }
        
        int target_index = -1; // Initialize the target index to -1 (no target)
        double closest_distance_so_far = t.range; // Initialize the minimum distance to the tower's range
        
        for(int k = 0; k < length(session.monsters); k++){ // Loop through all monsters to find the closest one within range
            monster &check_monster = session.monsters[k];
            if(check_monster.alive){
                double hypotenuse_distance = calculate_distance(t.x, t.y, check_monster.x, check_monster.y); // Calculate the distance between the tower and the monster
                
                if(hypotenuse_distance < closest_distance_so_far){
                    closest_distance_so_far = hypotenuse_distance; // Update the minimum distance if a closer monster is found
                    target_index = k; // Update the target index to the index of the closest monster
                }
            }
        }
    
        // If a target monster is found within range, apply damage to it
        if(target_index != -1){
            monster &target_monster = session.monsters[target_index];
            
            // Generate a bullet
            bullet b = create_bullet(t.x, t.y, target_monster.x, target_monster.y, target_monster.id, t.damage); // Create a bullet aimed at the target monster
            add(session.bullets, b); // Add the bullet to the bullets array
            t.cooldown = GAME_CONFIG.SUB_TOWER.ATTACK_COOLDOWN; // Reset the tower's cooldown timer based on its attack speed
        }
    }
}

// Function to build a new tower at the specified mouse position, if valid and affordable
bool build_tower(game_session &session, int mouse_x, int mouse_y){
    write_line("[INPUT DEBUG] Click detected at X: " + to_string(mouse_x) + " Y: " + to_string(mouse_y));

    int adjusted_x = mouse_x - GAME_CONFIG.OFFSET_X;
    int adjusted_y = mouse_y - GAME_CONFIG.OFFSET_Y;

    if(adjusted_x < 0 || adjusted_x >= GAME_CONFIG.TILE_SIZE * GAME_CONFIG.MAP_DIMENSION || adjusted_y < 0 || adjusted_y >= GAME_CONFIG.TILE_SIZE * GAME_CONFIG.MAP_DIMENSION){
        return false;
    }

    int col = adjusted_x / GAME_CONFIG.TILE_SIZE;
    int row = adjusted_y / GAME_CONFIG.TILE_SIZE;

    if(col < 0 || col >= GAME_CONFIG.MAP_DIMENSION || row < 0 || row >= GAME_CONFIG.MAP_DIMENSION){
        return false;
    }
    if(session.grid[row][col].single_tile != EMPTY_TILE){
        return false;
    }
    if(session.currency < GAME_CONFIG.SUB_TOWER.COST){
        return false;
    }

    session.currency -= GAME_CONFIG.SUB_TOWER.COST;
    session.grid[row][col].single_tile = SUB_TOWER_TILE;

    double pos_x = GAME_CONFIG.OFFSET_X + col * GAME_CONFIG.TILE_SIZE + GAME_CONFIG.TILE_SIZE / 2.0;
    double pos_y = GAME_CONFIG.OFFSET_Y + row * GAME_CONFIG.TILE_SIZE + GAME_CONFIG.TILE_SIZE / 2.0;
    tower new_sub_tower = create_sub_tower(pos_x, pos_y);

    add(session.towers, new_sub_tower);
    session.grid[row][col].tower_index = length(session.towers) - 1; // Store the index of the newly added tower in the grid for reference

    return true;
}

tower create_sub_tower(double pos_x, double pos_y){
    tower t;
    t.x = pos_x;
    t.y = pos_y;
    t.range = GAME_CONFIG.SUB_TOWER.RANGE;
    t.damage = GAME_CONFIG.SUB_TOWER.DAMAGE;
    t.attack_speed = GAME_CONFIG.SUB_TOWER.ATTACK_SPEED;
    t.cooldown = 0;
    t.is_main_tower = false;
    t.tower_health = GAME_CONFIG.SUB_TOWER.HEALTH;
    t.max_tower_health = GAME_CONFIG.SUB_TOWER.MAX_HEALTH;
    t.level = GAME_CONFIG.SUB_TOWER.INITIAL_LEVEL; // Set the initial level of the new sub-tower
    return t;
}

void apply_tower_upgrade(tower &t){
    t.level++; // Increase the tower's level
    t.damage += GAME_CONFIG.TOWER_UPGRADE.DAMAGE_INCREMENT; // Increase the tower's damage
    t.range += GAME_CONFIG.TOWER_UPGRADE.RANGE_INCREMENT; // Increase the tower's range
    t.tower_health += GAME_CONFIG.TOWER_UPGRADE.HEALTH_INCREMENT; // Increase the tower's health
    t.max_tower_health += GAME_CONFIG.TOWER_UPGRADE.HEALTH_INCREMENT; // Increase the tower's maximum health
    t.attack_speed += GAME_CONFIG.TOWER_UPGRADE.ATTACK_SPEED_INCREMENT; // Increase the tower's attack speed
}

// Function to check if a tower can be upgraded based on its index and current level
bool upgrade_tower(game_session &session, int tower_index){
    if(tower_index < 0 || tower_index >= length(session.towers)){
        write_line("[UPGRADE DEBUG] Fail: Invalid tower index " + to_string(tower_index));
        return false; // Invalid tower index
    }

    tower &t = session.towers[tower_index]; // Reference to the tower to be upgraded
    if(t.level >= GAME_CONFIG.SUB_TOWER.MAX_LEVEL || t.is_main_tower){
        write_line("[UPGRADE DEBUG] Fail: Main tower or already MAX level!");
        return false; // The tower is a main tower or already at max level
    }

    int upgrade_cost = GAME_CONFIG.TOWER_UPGRADE.BASE_UPGRADE_COST * (t.level + 1); // Calculate the cost of upgrading based on the tower's current level
    if(session.currency < upgrade_cost){
        write_line("[UPGRADE DEBUG] Fail: Not enough currency! Need: " + to_string(upgrade_cost) + ", Have: " + to_string(session.currency));
        return false; // Not enough currency to upgrade the tower
    }

    session.currency -= upgrade_cost; // Deduct the upgrade cost from the player's currency
    apply_tower_upgrade(t); // Apply the upgrade to the tower's properties
    
    write_line("[UPGRADE DEBUG] Success! Tower [" + to_string(tower_index) + "] upgraded to Lv." + to_string(t.level) + " !");

    return true;
}

bullet create_bullet(double start_x, double start_y, double target_monster_x, double target_monster_y, int target_monster_id, double damage){
    bullet b;
    b.x = start_x;
    b.y = start_y;
    b.target_monster_id = target_monster_id; // Set the target monster ID for the bullet
    b.speed = GAME_CONFIG.BULLET.SPEED; // Set the speed of the bullet
    b.damage = damage; // Set the damage of the bullet
    b.active = true; // Mark the bullet as active

    double distance_to_target = calculate_distance(start_x, start_y, target_monster_x, target_monster_y); // Calculate the distance to the target monster at the instant the bullet is created
    if(distance_to_target > 0){
        b.dx = (target_monster_x - start_x) / distance_to_target; // Calculate the normalized x direction towards the target monster
        b.dy = (target_monster_y - start_y) / distance_to_target; // Calculate the normalized y direction towards the target monster
    }
    else{
        b.dx = 0;
        b.dy = 0;
    }

    return b; // Return the created bullet
}

// Function to check if a bullet collides with a monster based on their positions and the monster's hitbox radius
bool bullet_collides_with_monster(const bullet &b, const monster &m){
    double hitbox_radius = GAME_CONFIG.MONSTER.RADIUS; // Define the hitbox radius for the monster

    double max_closing_distance_per_frame = b.speed + GAME_CONFIG.MONSTER.SPEED; // Calculate the maximum distance the bullet and monster can close in one frame

    double hit_threshold = hitbox_radius + max_closing_distance_per_frame; // Calculate the hit threshold based on the monster's hitbox radius and the maximum closing distance

    return calculate_distance(b.x, b.y, m.x, m.y) <= hit_threshold;
}

// Procedure to update the position and state of each bullet in the game session
void update_bullet(game_session &session){
    for(int i = 0; i < length(session.bullets); i++){
        bullet &b = session.bullets[i];
        if(!b.active) continue; // Skip inactive bullets

        // Update the bullet's position based on its direction and speed, does not change anymore after creation
        b.x += b.dx * b.speed;
        b.y += b.dy * b.speed;

        bool hit = false; // Flag to indicate if the bullet has hit a monster

        // Iterate through all monsters to check for collisions with the bullet
        for(int k = 0; k < length(session.monsters); k++){
            monster &m = session.monsters[k];
            if(!m.alive) continue; // Skip dead monsters

            // If the distance is less than or equal to the hit threshold
            if(bullet_collides_with_monster(b, m)){
                m.monster_health -= b.damage; // Apply damage to the monster's health
                if(m.monster_health <= 0){
                    m.alive = false;
                    write_line("[DEBUG] Monster killed! Current array length: " + to_string(length(session.monsters)));
                    session.currency += GAME_CONFIG.MONSTER.KILL_REWARD;
                }
                b.active = false; // Once a bullet hits a monster, it becomes inactive and will be removed in the next cleanup cycle
                hit = true; // Set the hit flag to true to indicate that the bullet has hit a monster
                break; // Exit the loop since the bullet can only hit one monster at a time
            }
        }

        if(hit) continue; // Skip further checks for this bullet if it has already hit a monster

        // If the bullet goes out of the screen bounds, mark it as inactive to be removed in the next cleanup cycle
        if(b.x < GAME_CONFIG.MAP_LEFT || b.x > GAME_CONFIG.MAP_RIGHT || b.y < GAME_CONFIG.MAP_TOP || b.y > GAME_CONFIG.MAP_BOTTOM) {
            b.active = false;
        }
    }
} 

// Procedure to remove inactive bullets from the game session's bullets array
void remove_inactive_bullet(game_session &session){
    for (int i = length(session.bullets) - 1; i >= 0; i--) {
        if (!session.bullets[i].active) {
            remove_at(session.bullets, i);
        }
    }
}


// Procedure to remove dead monsters from the game session's monsters array
void remove_dead_monster(game_session &session){
    for(int i = length(session.monsters) - 1; i >= 0; i--){ // Iterate backwards to safely remove dead monsters
        if(!session.monsters[i].alive){
            remove_at(session.monsters, i); // Remove dead monsters from the monsters array
            write_line("[DEBUG] Dead monster removed! Current array length: " + to_string(length(session.monsters))); // Log the removal of a monster
        }
    }
}

// Procedure to update the wave of monsters, spawning new monsters and preparing for the next wave
void update_wave(game_session &session){
        // Increase the spawn timer if there are still monsters left to spawn in the current wave
        if(session.monster_left_to_spawn > 0){
        session.spawn_timer++;

        double current_interval = GAME_CONFIG.SESSION.INITIAL_SPAWN_INTERVAL - (session.wave_number - 1) * GAME_CONFIG.SESSION.SPAWN_INTERVAL_DECREASE_PER_WAVE;
        if(current_interval < GAME_CONFIG.SESSION.MIN_SPAWN_INTERVAL){
            current_interval = GAME_CONFIG.SESSION.MIN_SPAWN_INTERVAL;
        }

        // Check if it's time to spawn a new monster based on the spawn timer and interval
        if(session.spawn_timer >= current_interval){
            session.spawn_timer = 0.0; // Reset the spawn timer    
            spawn_monster(session); // Spawn a new monster
            session.monster_left_to_spawn--;
        }
    }
    
    // Check if all monsters have been spawned and defeated, and prepare for the next wave
    else if(session.monster_left_to_spawn <= 0 && length(session.monsters) == 0){
        write_line("[DEBUG] Wave completed! Switching to wave: " + to_string(session.wave_number + 1)); // Log the completion of the current wave
        delay(GAME_CONFIG.SESSION.WAVE_TRANSITION_DELAY_MS); // Delay for 1 second before starting the next wave
        session.wave_number++;
        session.monster_left_to_spawn = session.wave_number * GAME_CONFIG.SESSION.INITIAL_MONSTER_LEFT_TO_SPAWN; // Calculate the number of monsters for the next wave
        session.spawn_timer = 0.0; // Reset the spawn timer for the next wave
    }
}

// Function to check if the game is over based on the main tower's health
bool is_game_over(game_session &session){
    for(int i = 0; i < length(session.towers); i++){
        // Check if the current tower is the main tower and if its health is zero or below
        if(session.towers[i].is_main_tower && session.towers[i].tower_health <= 0){
            session.is_game_over = true; // Set the game over flag if the main tower's health is zero or below
            return true;
        }
        return false; // Return false if the main tower is still alive
    }
    return false; // Return false if there are no towers in the session (should not happen in normal gameplay)
}

// Function to handle user input, specifically mouse clicks for building or upgrading towers
void handle_input(game_session &session){
    if (mouse_clicked(LEFT_BUTTON)) {
        int adjusted_x = mouse_x() - GAME_CONFIG.OFFSET_X;
        int adjusted_y = mouse_y() - GAME_CONFIG.OFFSET_Y;

        if (adjusted_x < 0 || adjusted_y < 0) return;

        int col = adjusted_x / GAME_CONFIG.TILE_SIZE;
        int row = adjusted_y / GAME_CONFIG.TILE_SIZE;

        if (row >= 0 && row < GAME_CONFIG.MAP_DIMENSION && col >= 0 && col < GAME_CONFIG.MAP_DIMENSION) {
            tile &t = session.grid[row][col];

            // Case A: Clicked on a sub-tower tile -> Upgrade the tower if possible
            if (t.single_tile == SUB_TOWER_TILE && t.tower_index != -1) {
                upgrade_tower(session, t.tower_index);
            } 
            // Case B: Clicked on an empty tile -> Build a new tower
            else if (t.single_tile == EMPTY_TILE) {
                build_tower(session, mouse_x(), mouse_y());
            }
        }
    }
}
