#include "friends.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TEXT_SEPR "------------------------------------------\r\n"
#define NEWLINE_CHAR "\r\n" // can be changed to \n if we want

void *Malloc(size_t num_bytes){
	void *ret = malloc(num_bytes);
	if (ret == NULL){
		perror("malloc");
		exit(1);
	}
	return ret;
}

/*
 * Create a new user with the given name.  Insert it at the tail of the list
 * of users whose head is pointed to by *user_ptr_add.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if a user by this name already exists in this list.
 *   - 2 if the given name cannot fit in the 'name' array
 *       (don't forget about the null terminator).
 */
int create_user(const char *name, User **user_ptr_add) {
    if (strlen(name) >= MAX_NAME) {
        return 2;
    }

    User *new_user = malloc(sizeof(User));
    if (new_user == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_user->name, name, MAX_NAME); // name has max length MAX_NAME - 1

    for (int i = 0; i < MAX_NAME; i++) {
        new_user->profile_pic[i] = '\0';
    }

    new_user->first_post = NULL;
    new_user->next = NULL;
    for (int i = 0; i < MAX_FRIENDS; i++) {
        new_user->friends[i] = NULL;
    }

    // Add user to list
    User *prev = NULL;
    User *curr = *user_ptr_add;
    while (curr != NULL && strcmp(curr->name, name) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (*user_ptr_add == NULL) {
        *user_ptr_add = new_user;
        return 0;
    } else if (curr != NULL) {
        free(new_user);
        return 1;
    } else {
        prev->next = new_user;
        return 0;
    }
}


/*
 * Return a pointer to the user with this name in
 * the list starting with head. Return NULL if no such user exists.
 *
 * NOTE: You'll likely need to cast a (const User *) to a (User *)
 * to satisfy the prototype without warnings.
 */
User *find_user(const char *name, const User *head) {
    while (head != NULL && strcmp(name, head->name) != 0) {
        head = head->next;
    }

    return (User *)head;
}


/*
 * Print the usernames of all users in the list starting at curr.
 * Names should be printed to standard output, one per line.
 */
char *list_users(const User *curr) {
    char *header = "User List";
    int num_chars = strlen(header) + strlen(NEWLINE_CHAR);

    User *curr_user = (User *) curr;
    while (curr_user != NULL){
        num_chars += strlen(curr_user->name) + strlen(NEWLINE_CHAR) + 1; // +1 for tab
        curr_user = curr_user->next;
    }

    char *user_list = Malloc(sizeof(char) * (num_chars + 1)); // extra character for null character
    memset(user_list, '\0', num_chars + 1); // fill array with null characters

    strncat(user_list, header, strlen(header) + 1);
    strncat(user_list, NEWLINE_CHAR, strlen(NEWLINE_CHAR) + 1);
    curr_user = (User *) curr;
    while (curr_user != NULL){
        strncat(user_list, "\t", 2);
        strncat(user_list, curr_user->name, strlen(curr_user->name));
        strncat(user_list, NEWLINE_CHAR, strlen(NEWLINE_CHAR) + 1);
        curr_user = curr_user->next;
    }

    return user_list;
}


/*
 * Make two users friends with each other.  This is symmetric - a pointer to
 * each user must be stored in the 'friends' array of the other.
 *
 * New friends must be added in the first empty spot in the 'friends' array.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if the two users are already friends.
 *   - 2 if the users are not already friends, but at least one already has
 *     MAX_FRIENDS friends.
 *   - 3 if the same user is passed in twice.
 *   - 4 if at least one user does not exist.
 *
 * Do not modify either user if the result is a failure.
 * NOTE: If multiple errors apply, return the *largest* error code that applies.
 */
int make_friends(const char *name1, const char *name2, User *head) {
    User *user1 = find_user(name1, head);
    User *user2 = find_user(name2, head);

    if (user1 == NULL || user2 == NULL) {
        return 4;
    } else if (user1 == user2) { // Same user
        return 3;
    }

    int i, j;
    for (i = 0; i < MAX_FRIENDS; i++) {
        if (user1->friends[i] == NULL) { // Empty spot
            break;
        } else if (user1->friends[i] == user2) { // Already friends.
            return 1;
        }
    }

    for (j = 0; j < MAX_FRIENDS; j++) {
        if (user2->friends[j] == NULL) { // Empty spot
            break;
        }
    }

    if (i == MAX_FRIENDS || j == MAX_FRIENDS) { // Too many friends.
        return 2;
    }

    user1->friends[i] = user2;
    user2->friends[j] = user1;
    return 0;
}


/*
 *  Print a post.
 *  Use localtime to print the time and date.
 *  Returned string has newline character at the end.
 */
char *print_post(const Post *post) {
    if (post == NULL) {
        return NULL;
        // return 1;
    }

    int num_chars = strlen("From: ") + strlen(post->author) + strlen(NEWLINE_CHAR) + \
    strlen("Date: ") + strlen(asctime(localtime(post->date))) + strlen(NEWLINE_CHAR) + \
    strlen(post->contents) + strlen(NEWLINE_CHAR) + 1; // + 1 for null character at the end

    char *post_str = Malloc(sizeof(char) * num_chars);
    memset(post_str, '\0', num_chars);

    snprintf(post_str, num_chars, "From: %s%sDate: %s%s%s%s", 
        post->author, 
        NEWLINE_CHAR,
        asctime(localtime(post->date)), 
        NEWLINE_CHAR,
        post->contents,
        NEWLINE_CHAR
    );

    return post_str;

    // Print author
    // printf("From: %s\n", post->author);

    // // Print date
    // printf("Date: %s\n", asctime(localtime(post->date)));

    // // Print message
    // printf("%s\n", post->contents);

    // return 0;
}


/*
 * Print a user profile.
 * For an example of the required output format, see the example output
 * linked from the handout.
 * Return:
 *   - 0 on success.
 *   - 1 if the user is NULL.
 */
char *print_user(const User *user) {
    if (user == NULL) {
        return NULL;
    }

    // definitely have to print at least these characters
    // Name:name\n\nFriends:<friends_list>\nPosts:<posts>\n
    int num_chars = strlen("Name:") + strlen("Friends:") + strlen("Posts:") + 3 * strlen(TEXT_SEPR) + 4 * strlen(NEWLINE_CHAR);
    num_chars += strlen(user->name);

    int i = 0;
    while ((user->friends)[i] != NULL) {
        num_chars += strlen((user->friends)[i]->name) + 1; // + 1 for newline character
        i++;
    }

    Post *curr_post = user->first_post;
    while (curr_post != NULL) {
        char *post_str = print_post(curr_post);
        num_chars += strlen(post_str);
        free(post_str);
        curr_post = curr_post->next;

        if (curr_post != NULL) {
            num_chars += strlen("===");
            num_chars += 3 * strlen(NEWLINE_CHAR);
        }
    }

    char *user_str = malloc(sizeof(char) * num_chars);
    memset(user_str, '\0', num_chars);  // \n\n          \n
    snprintf(user_str, num_chars, "Name: %s%s%s%sFriends:%s", 
        user->name, 
        NEWLINE_CHAR,
        NEWLINE_CHAR,
        TEXT_SEPR,
        NEWLINE_CHAR
    );

    i = 0;
    while ((user->friends)[i] != NULL) {
        strncat(user_str, ((user->friends)[i])->name, strlen(((user->friends)[i])->name) + 1);
        strncat(user_str, NEWLINE_CHAR, strlen(NEWLINE_CHAR) + 1);
        i++;
    }
    strncat(user_str, TEXT_SEPR, strlen(TEXT_SEPR) + 1);
    strncat(user_str, "Posts:", strlen("Posts:") + 1);
    strncat(user_str, NEWLINE_CHAR, strlen(NEWLINE_CHAR) + 1);
    curr_post = user->first_post;
    while (curr_post != NULL) {
        char *post_str2 = print_post(curr_post);
        strncat(user_str, post_str2, strlen(post_str2) + 1);
        free(post_str2); // don't need it after copying things over
        curr_post = curr_post->next;
        if (curr_post != NULL) {
            strncat(user_str, NEWLINE_CHAR, strlen(NEWLINE_CHAR) + 1);
            strncat(user_str, "===", 4);
            strncat(user_str, NEWLINE_CHAR, strlen(NEWLINE_CHAR) + 1);
            strncat(user_str, NEWLINE_CHAR, strlen(NEWLINE_CHAR) + 1);
        }
    }
    strncat(user_str, TEXT_SEPR, strlen(TEXT_SEPR) + 1);

    return user_str;


    // Print name
    // printf("Name: %s\n\n", user->name);
    // printf("------------------------------------------\n");

    // // Print friend list.
    // printf("Friends:\n");
    // for (int i = 0; i < MAX_FRIENDS && user->friends[i] != NULL; i++) {
    //     printf("%s\n", user->friends[i]->name);
    // }
    // printf("------------------------------------------\n");

    // // Print post list.
    // printf("Posts:\n");
    // const Post *curr = user->first_post;
    // while (curr != NULL) {
    //     print_post(curr);
    //     curr = curr->next;
    //     if (curr != NULL) {
    //         printf("\n===\n\n");
    //     }
    // }
    // printf("------------------------------------------\n");

    // return 0;
}


/*
 * Make a new post from 'author' to the 'target' user,
 * containing the given contents, IF the users are friends.
 *
 * Insert the new post at the *front* of the user's list of posts.
 *
 * Use the 'time' function to store the current time.
 *
 * 'contents' is a pointer to heap-allocated memory - you do not need
 * to allocate more memory to store the contents of the post.
 *
 * Return:
 *   - 0 on success
 *   - 1 if users exist but are not friends
 *   - 2 if either User pointer is NULL
 */
int make_post(const User *author, User *target, char *contents) {
    if (target == NULL || author == NULL) {
        return 2;
    }

    int friends = 0;
    for (int i = 0; i < MAX_FRIENDS && target->friends[i] != NULL; i++) {
        if (strcmp(target->friends[i]->name, author->name) == 0) {
            friends = 1;
            break;
        }
    }

    if (friends == 0) {
        return 1;
    }

    // Create post
    Post *new_post = malloc(sizeof(Post));
    if (new_post == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_post->author, author->name, MAX_NAME);
    new_post->contents = contents;
    new_post->date = malloc(sizeof(time_t));
    if (new_post->date == NULL) {
        perror("malloc");
        exit(1);
    }
    time(new_post->date);
    new_post->next = target->first_post;
    target->first_post = new_post;

    return 0;
}

