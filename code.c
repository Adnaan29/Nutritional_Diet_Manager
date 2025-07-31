#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define TABLE_SIZE 100
#define MAX_USERS 100
#define MAX_MEALS 500
#define MAX_MEAL_HISTORY 5
#define CREDENTIALS_FILE "users.txt"
#define MEALS_FILE "meals_updated.csv"
#define BREAKFAST_PERCENT 0.25
#define LUNCH_PERCENT 0.40
#define DINNER_PERCENT 0.30
#define SNACK_PERCENT 0.05

typedef struct {
    char name[50];
    char type[20];
    int calories;
} MealHistory;

typedef struct User {
    char username[50];
    char password[50];
    char name[50];
    float height, weight;
    int daily_calories;
    int remaining_calories;
    float bmi;
    char bmi_category[20];
    MealHistory last_meals[MAX_MEAL_HISTORY];
    int meal_count;
    struct User* next;
} User;

typedef struct Meal {
    char name[50];
    int calories;
    char type[20];
    struct Meal* next;
} Meal;

User* userList = NULL;
User* hashTable[TABLE_SIZE];
Meal allMeals[MAX_MEALS];
int mealCount = 0;

unsigned int hash(const char* key) {
    unsigned int hash_value = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        hash_value = 31 * hash_value + key[i];
    }   
    return hash_value % TABLE_SIZE;
}

void insert_user_to_hash_table(User* user) {
    unsigned int index = hash(user->username);
    user->next = hashTable[index];
    hashTable[index] = user;
}

User* find_user_in_hash_table(const char* username) {
    int index = hash(username);
    User* current = hashTable[index];
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void calculate_bmi(User* user) {
    user->bmi = user->weight / ((user->height / 100) * (user->height / 100));
    if (user->bmi < 18.5) strcpy(user->bmi_category, "Underweight");
    else if (user->bmi < 25 && user->bmi >= 18.5) strcpy(user->bmi_category, "Normal Weight");
    else if (user->bmi < 30 && user->bmi >= 25) strcpy(user->bmi_category, "Overweight");
    else strcpy(user->bmi_category, "Obese");
}

User* create_user(char username[], char password[], char name[], float height, float weight, int daily_calories) {
    User* new_user = (User*)malloc(sizeof(User));
    strcpy(new_user->username, username);
    strcpy(new_user->password, password);
    strcpy(new_user->name, name);
    new_user->height = height;
    new_user->weight = weight;
    new_user->daily_calories = daily_calories;
    new_user->remaining_calories = daily_calories;
    new_user->meal_count = 0;
    new_user->next = NULL;
    calculate_bmi(new_user);
    return new_user;
}

void insert_user(User** head, User* new_user) {
    if (*head == NULL) {
        *head = new_user;
    } 
    else {
        User* temp = *head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_user;
    }
}

void add_meal_suggestion(User* user, const char* name, const char* type, int calories) {
    if (user->meal_count == MAX_MEAL_HISTORY) {
        for (int i = 1; i < MAX_MEAL_HISTORY; i++) {
            memcpy(&user->last_meals[i-1], &user->last_meals[i], sizeof(MealHistory));
        }
        user->meal_count--;
    }
    strcpy(user->last_meals[user->meal_count].name, name);
    strcpy(user->last_meals[user->meal_count].type, type);
    user->last_meals[user->meal_count].calories = calories;
    user->meal_count++;
    user->remaining_calories -= calories;
}

int is_meal_in_history(User* user, const char* meal_name) {
    for (int i = 0; i < user->meal_count; i++) {
        if (strcmp(user->last_meals[i].name, meal_name) == 0) {
            return 1;
        }
    }
    return 0;
}

bool parse_user_data(char* data, User* user) {
    return sscanf(data, "%s %s \"%[^\"]\" %f %f %d %d",
                 user->username, user->password, user->name,
                 &user->height, &user->weight, &user->daily_calories,
                 &user->meal_count) == 7;
}

void parse_meal_data(char* data, User* user, int meal_count) {
    char* meal = strtok(data, "|");
    for (int i = 0; meal && i < meal_count; i++) {
        char* name = strtok(meal, ",");
        char* type = strtok(NULL, ",");
        char* calories = strtok(NULL, ",");

        if (name && type && calories) {
            strncpy(user->last_meals[i].name, name, 49);
            strncpy(user->last_meals[i].type, type, 19);
            user->last_meals[i].name[49] = '\0';
            user->last_meals[i].type[19] = '\0';
            user->last_meals[i].calories = atoi(calories);
            user->remaining_calories -= user->last_meals[i].calories;
            user->meal_count++;
        }
        meal = strtok(NULL, "|");
    }
}

void load_users_from_file() {
    FILE* file = fopen(CREDENTIALS_FILE, "r");
    if (!file) return;

    char line[200];
    while (fgets(line, sizeof(line), file)) {

        char* meal_data = strchr(line, '|');
        if (meal_data) {
            *meal_data = '\0';  
            meal_data++;       
        }

        User user = {0};
        if (parse_user_data(line, &user) ){
            User* new_user = create_user(user.username, user.password, user.name,
                                       user.height, user.weight, user.daily_calories);
            new_user->remaining_calories = user.daily_calories;

            if (meal_data && user.meal_count > 0) {
                parse_meal_data(meal_data, new_user, user.meal_count);
            }

            insert_user(&userList, new_user);
            insert_user_to_hash_table(new_user);
        }
    }
    fclose(file);
}

void save_users_to_file() {
    FILE* file = fopen(CREDENTIALS_FILE, "w");
    if (!file) return;

    User* current = userList;
    while (current) {
        fprintf(file, "%s %s \"%s\" %.2f %.2f %d %d",
               current->username, current->password, current->name,
               current->height, current->weight, current->daily_calories,
               current->meal_count);

        if (current->meal_count > 0) {
            fputc('|', file);
            for (int i = 0; i < current->meal_count; i++) {
                fprintf(file, "%s,%s,%d",
                       current->last_meals[i].name,
                       current->last_meals[i].type,
                       current->last_meals[i].calories);
                if (i < current->meal_count - 1) fputc('|', file);
            }
        }
        fputc('\n', file);
        current = current->next;
    }
    fclose(file);
}

void load_all_meals() {
    FILE* file = fopen(MEALS_FILE, "r");
    if (!file) return;

    char line[200];
    while (fgets(line, sizeof(line), file) && mealCount < MAX_MEALS) {
        line[strcspn(line, "\n")] = 0;
        char* name = strtok(line, ",");
        char* calStr = strtok(NULL, ",");
        char* type = strtok(NULL, ",");
        
        if (name && calStr && type) {
            while (*type == ' ') type++;
            strcpy(allMeals[mealCount].name, name);
            allMeals[mealCount].calories = atoi(calStr);
            strcpy(allMeals[mealCount].type, type);
            mealCount++;
        }
    }
    fclose(file);
}

void display_meals_by_type(const char* type) {
    printf("\nAll %s Meals:\n", type);
    int count = 0;
    
    for (int i = 0; i < mealCount; i++) {
        if (strcasecmp(allMeals[i].type, type) == 0) {
            printf("%d. %s (%d cal)\n", ++count, allMeals[i].name, allMeals[i].calories);
        }
    }
    
    if (count == 0) {
        printf("No meals found.\n");
    }
}

Meal* get_top_meals(const char* type, int min_cal, int max_cal, int count, User* user) {
    Meal eligible_meals[MAX_MEALS];
    int eligible_count = 0;
    
    for (int i = 0; i < mealCount; i++) {
        if (strcasecmp(allMeals[i].type, type) == 0 && allMeals[i].calories >= min_cal && allMeals[i].calories <= max_cal && (user == NULL || !is_meal_in_history(user, allMeals[i].name))) {
            
            eligible_meals[eligible_count++] = allMeals[i];
        }
    }
    
    if (eligible_count <= count) {
        Meal* result = NULL;
        for (int i = 0; i < eligible_count; i++) {
            Meal* new_meal = (Meal*)malloc(sizeof(Meal));
            *new_meal = eligible_meals[i];
            new_meal->next = result;
            result = new_meal;
        }
        return result;
    }
    
    Meal* result = NULL;
    int selected_indices[MAX_MEALS] = {0};
    srand(time(NULL));
    
    for (int i = 0; i < count; i++) {
        int random_index;
        do {
            random_index = rand() % eligible_count;
        } while (selected_indices[random_index]);
        
        selected_indices[random_index] = 1;
        
        Meal* new_meal = (Meal*)malloc(sizeof(Meal));
        *new_meal = eligible_meals[random_index];
        new_meal->next = result;
        result = new_meal;
    }
    
    return result;
}

void suggest_and_choose_meal(User* user, const char* type) {
    int target_calories = 0;
    int max_calories = 0;
    
    if (strcmp(type, "Breakfast") == 0) {
        target_calories = user->daily_calories * BREAKFAST_PERCENT;
        max_calories = user->remaining_calories * 0.6; 
    } else if (strcmp(type, "Lunch") == 0) {
        target_calories = user->daily_calories * LUNCH_PERCENT;
        max_calories = user->remaining_calories * 0.6;
    } else if (strcmp(type, "Dinner") == 0) {
        target_calories = user->daily_calories * DINNER_PERCENT;
        max_calories = user->remaining_calories * 0.6;
    } else { /// Snacks
        target_calories = user->daily_calories * SNACK_PERCENT;
        max_calories = user->remaining_calories;
    }
    
    float bmi_factor = 1.0;
    if (user->bmi < 18.5) bmi_factor = 1.1; 
    else if (user->bmi >= 25 && user->bmi <= 30) bmi_factor = 0.95;
    else bmi_factor = 0.9;
    
    target_calories *= bmi_factor;
    
    if (target_calories > user->remaining_calories) {
        target_calories = user->remaining_calories;
    }
    
    int min_cal = target_calories * 0.7;
    int max_cal = target_calories * 1.3;
    if (max_cal > max_calories) max_cal = max_calories;
    
    Meal* options = get_top_meals(type, min_cal, max_cal, 3, user);
    if (options == NULL) {
        printf("\nNo %s options found.\n", type);
        return;
    }

    printf("\n%s Options (Remaining cal : %d):\n", type, user->remaining_calories);
    int i = 1;
    for (Meal* current = options; current != NULL ; current = current->next) {
        printf("%d. %s (%d cal)\n", i++, current->name, current->calories);
    }

    printf("\nChoose option: ");
    int choice;
    while (scanf("%d", &choice) != 1 || choice < 1 || choice > 3) {
        printf("Invalid choice entered. ");
        while (getchar() != '\n');
    }

    Meal* selected = options;
    for (int j = 1; j < choice && selected; j++) {
        selected = selected->next;
    }

    if (selected) {
        printf("\nYou have selected %s (%d cal)\n", selected->name, selected->calories);
        add_meal_suggestion(user, selected->name, type, selected->calories);
        save_users_to_file();
    }

    while (options) {
        Meal* temp = options;
        options = options->next;
        free(temp);
    }
}

void suggest_meals(User* user) {
    printf("\nRemaining Daily Calories: %d/%d\n", user->remaining_calories, user->daily_calories);
    printf("\nSelect Meal Type to Get Suggestions:\n");
    printf("1. Breakfast\n2. Lunch\n3. Dinner\n4. Snack\n");
    printf("Enter choice: ");
    
    int choice;
    scanf("%d", &choice);
    
    const char* type;
    switch(choice) {
        case 1: type = "Breakfast"; break;
        case 2: type = "Lunch"; break;
        case 3: type = "Dinner"; break;
        case 4: type = "Evening Snacks"; break;
        default:
            printf("Invalid choice entered.\n");
            return;
    }
    
    suggest_and_choose_meal(user, type);
}

void register_user() {
    char username[50], password[50], name[50];
    float height, weight;
    int daily_calories;

    printf("Enter a new username: ");
    scanf("%s", username);
    if (find_user_in_hash_table(username) != NULL) {
        printf("Username already exists.\n");
        return;
    }
    printf("Enter a password: ");
    scanf("%s", password);
    printf("Enter Name: ");
    getchar();
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;
    printf("Enter Height (cm): ");
    scanf("%f", &height);
    printf("Enter Weight (kg): ");
    scanf("%f", &weight);
    printf("Enter Daily Calorie Goal: ");
    scanf("%d", &daily_calories);

    User* new_user = create_user(username, password, name, height, weight, daily_calories);
    insert_user(&userList, new_user);
    insert_user_to_hash_table(new_user);
    save_users_to_file();
    printf("Registration successful!\n");
}

User* login_user() {
    char username[50], password[50];
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);
    User* user = find_user_in_hash_table(username);
    if (user != NULL && strcmp(user->password, password) == 0) {
        printf("Login successful!\n");
        return user;
    }
    printf("Invalid username or password\n");
    return NULL;
}

void update_user_details(User* user) {
    printf("Update Your Details:\n");
    printf("Enter Height (cm): ");
    scanf("%f", &user->height);
    printf("Enter Weight (kg): ");
    scanf("%f", &user->weight);
    printf("Enter Daily Calorie Goal: ");
    scanf("%d", &user->daily_calories);
    calculate_bmi(user);
    
    user->remaining_calories = user->daily_calories;
    save_users_to_file();
    printf("Details updated successfully!\n");
}

void display_user_details(User* user) {
    printf("\nUser Details:\n");
    printf("Username: %s\n", user->username);
    printf("Name: %s\n", user->name);
    printf("Height: %.2f cm\n", user->height);
    printf("Weight: %.2f kg\n", user->weight);
    printf("Daily Calorie Goal: %d\n", user->daily_calories);
    printf("Remaining Calories: %d\n", user->remaining_calories);
    printf("BMI: %.2f (%s)\n", user->bmi, user->bmi_category);
    
    printf("\nToday's Meals:\n");
    if (user->meal_count == 0) {
        printf("No meals recorded today\n");
    } else {
        int total_consumed = 0;
        for (int i = 0; i < user->meal_count; i++) {
            printf("%d. %s (%s) - %d cal\n", i+1, 
                  user->last_meals[i].name, 
                  user->last_meals[i].type,
                  user->last_meals[i].calories);
            total_consumed += user->last_meals[i].calories;
        }
        printf("\nTotal consumed today: %d cal\n", total_consumed);
    }
}

void print_divider(char ch, int width) {
    for (int i = 0; i < width; i++) {
        putchar(ch);
    }
    putchar('\n');
}

void print_banner(const char* title) {
    printf("\n");
    print_divider('=', 50);
    printf("%*s\n", (50 + strlen(title)) / 2, title);
    print_divider('=', 50);
}

void reset_daily_calories(User* user) {
    user->remaining_calories = user->daily_calories;
    user->meal_count = 0;
    save_users_to_file();
    printf("Daily calories reset. ready for a new day\n");
}

void mainMenu(User* currentUser) {
    int mainChoice;
    while (1) {
        print_banner("MAIN MENU");
        printf("\n 1. View Meal Suggestions\n 2. View All Dishes\n 3. About You\n 4. Update Your Details\n 5. Reset Daily Calories\n 6. Log Out\n");
        printf("Enter choice: ");
        scanf("%d", &mainChoice);

        switch (mainChoice) {
            case 1:
                print_banner("MEAL SUGGESTIONS");
                suggest_meals(currentUser);
                break;
            case 2: {
                print_banner("VIEW DISHES BY TYPE");
                printf("\n 1. Breakfast\n 2. Lunch\n 3. Dinner\n 4. Snacks\n");
                printf("Enter choice: ");
                int viewChoice;
                scanf("%d", &viewChoice);
                switch(viewChoice) {
                    case 1: display_meals_by_type("Breakfast"); break;
                    case 2: display_meals_by_type("Lunch"); break;
                    case 3: display_meals_by_type("Dinner"); break;
                    case 4: display_meals_by_type("Evening Snacks"); break;
                    default: printf("Invalid choice is entered\n");
                }
                break;
            }
            case 3:
                print_banner("YOUR PROFILE");
                display_user_details(currentUser);
                break;
            case 4:
                print_banner("UPDATE PROFILE");
                update_user_details(currentUser);
                break;
            case 5:
                print_banner("RESET DAILY CALORIES");
                reset_daily_calories(currentUser);
                break;
            case 6:
                print_banner("LOG OUT");
                printf("Logging out...\n");
                return;
            default:
                printf("Invalid choice is entered.\n");
        }
    }
}

int main() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        hashTable[i] = NULL;
    }

    load_users_from_file();
    load_all_meals();

    int choice;
    User* currentUser = NULL;

    while (1) {
        print_banner("NUTRITIONAL DIET MANAGER");
        printf("\n 1. Register\n 2. Login\n 3. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                print_banner("REGISTER NEW USER");
                register_user();
                break;
            case 2:
                print_banner("USER LOGIN");
                currentUser = login_user();
                if (currentUser != NULL) {
                    mainMenu(currentUser);
                }
                break;
            case 3:
                print_banner("EXIT PROGRAM");
                User* current = userList;
                while (current != NULL) {
                    User* temp = current;
                    current = current->next;
                    free(temp);
                }
                printf("Exiting program...\n");
                return 0;
            default:
                printf("Invalid choice\n");
        }
    }
}