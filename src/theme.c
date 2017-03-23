/* yuk
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include <stdlib.h>
#include <string.h>

#include "theme.h"

ThemeList *first_theme = NULL;
ThemeList *last_theme = NULL;

Theme *new_theme() {
  Theme *theme = malloc(sizeof(Theme));
  theme->name = NULL;
  theme->title = NULL;
  theme->heart = "H";
  theme->spade = "S";
  theme->diamond = "D";
  theme->club = "C";
  theme->width = 6;
  theme->height = 4;
  theme->x_spacing = 2;
  theme->y_spacing = 1;
  theme->x_margin = 2;
  theme->y_margin = 1;
  return theme;
}

void register_theme(Theme *theme) {
  if (theme->name) {
    ThemeList *next = malloc(sizeof(struct theme_list));
    next->theme = theme;
    next->next = NULL;
    if (last_theme) {
      last_theme->next = next;
      last_theme = next;
    } else {
      first_theme = last_theme = next;
    }
  }
}

ThemeList *list_themes() {
  return first_theme;
}

Theme *get_theme(const char *name) {
  for (ThemeList *themes = list_themes(); themes; themes = themes->next) {
    if (strcmp(themes->theme->name, name) == 0) {
      return themes->theme;
    }
  }
  return NULL;
}

char *card_suit(Card *card, Theme *theme) {
  switch (card->suit) {
    case HEART:
      return theme->heart;
    case SPADE:
      return theme->spade;
    case DIAMOND:
      return theme->diamond;
    case CLUB:
      return theme->club;
  }
  return "";
}