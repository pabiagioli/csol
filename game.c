/* yuk
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "card.h"

GameList *first_game = NULL;
GameList *last_game = NULL;

Game *new_game() {
  Game *game = malloc(sizeof(Game));
  game->name = NULL;
  game->title = NULL;
  game->first_rule = NULL;
  game->last_rule = NULL;
  return game;
}

GameRule *new_game_rule(GameRuleType type) {
  GameRule *rule = malloc(sizeof(GameRule));
  rule->next = NULL;
  rule->type = type;
  rule->x = 0;
  rule->y = 0;
  rule->deal = 0;
  rule->redeals = 0;
  rule->hide = 0;
  rule->first_suit = SUIT_ANY;
  rule->first_rank = RANK_ANY;
  rule->next_suit = SUIT_ANY;
  rule->next_rank = RANK_ANY;
  rule->move_group = MOVE_ONE;
  rule->from = RULE_ANY;
  switch (type) {
    case RULE_FOUNDATION:
      rule->first_rank = RANK_ACE;
      rule->next_suit = SUIT_SAME;
      rule->next_rank = RANK_UP;
      break;
    case RULE_TABLEAU:
      rule->next_suit = SUIT_ANY;
      rule->next_rank = RANK_DOWN;
      break;
    case RULE_STOCK:
      rule->first_suit = SUIT_NONE;
      rule->first_rank = RANK_NONE;
      rule->next_suit = SUIT_NONE;
      rule->next_rank = RANK_NONE;
      break;
    case RULE_WASTE:
      rule->from = RULE_STOCK;
      break;
  }
  return rule;
}

void register_game(Game *game) {
  if (game->name) {
    GameList *next = malloc(sizeof(GameList));
    next->game = game;
    next->next = NULL;
    if (last_game) {
      last_game->next = next;
      last_game = next;
    } else {
      first_game = last_game = next;
    }
  }
}

GameList *list_games() {
  return first_game;
}

Game *get_game(const char *name) {
  for (GameList *games = list_games(); games; games = games->next) {
    if (strcmp(games->game->name, name) == 0) {
      return games->game;
    }
  }
  return NULL;
}

Card *new_pile(GameRule *rule) {
  char rank = 0;
  if (rule->first_rank <= RANK_KING) {
    rank = (char)rule->first_rank;
  }
  switch (rule->type) {
    case RULE_TABLEAU:
      return new_card(TABLEAU, rank);
    case RULE_STOCK:
    case RULE_FOUNDATION:
    case RULE_WASTE:
      return new_card(FOUNDATION, rank);
  }
}

void deal_pile(Card *stack, GameRule *rule, Card *deck) {
  if (rule->deal > 0) {
    for (int i = 0; i < rule->deal && deck->next; i++) {
      move_stack(stack, take_card(deck->next));
    }
    if (rule->hide > 0) {
      Card *card = stack->next;
      for (int i = 0; i < rule->hide && card; i++, card = card->next) {
        card->up = 0;
      }
      for (; card; card = card->next) {
        card->up = 1;
      }
    } else if (rule->hide < 0) {
      Card *card = get_top(stack);
      int hide = -rule->hide;
      for (int i = 0; i < hide && !(card->suit & BOTTOM); i++, card = card->prev) {
        card->up = 1;
      }
      for (; !(card->suit & BOTTOM); card = card->prev) {
        card->up = 0;
      }
    }
  }
}

void delete_piles(Pile *piles) {
  if (piles->next) {
    delete_piles(piles->next);
  }
  delete_stack(piles->stack);
  free(piles);
}

Pile *deal_cards(Game *game, Card *deck) {
  Pile *first = NULL;
  Pile *last = NULL;
  for (GameRule *rule = game->first_rule; rule; rule = rule->next) {
    Pile *pile = malloc(sizeof(Pile));
    pile->next = NULL;
    pile->rule = rule;
    pile->stack = new_pile(rule);
    pile->redeals = 0;
    deal_pile(pile->stack, pile->rule, deck);
    if (last) {
      last->next = pile;
      last = pile;
    } else {
      first = last = pile;
    }
  }
  return first;
}

int check_first_suit(Card *card, GameRuleSuit suit) {
  switch (suit) {
    case SUIT_NONE:
      return 0;
    case SUIT_ANY:
      return 1;
    case SUIT_HEART:
      return card->suit == HEART;
    case SUIT_DIAMOND:
      return card->suit == DIAMOND;
    case SUIT_SPADE:
      return card->suit == SPADE;
    case SUIT_CLUB:
      return card->suit == CLUB;
    case SUIT_RED:
      return card->suit & RED;
    case SUIT_BLACK:
      return card->suit & BLACK;
  }
  return 0;
}

int check_first_rank(Card *card, GameRuleRank rank) {
  if (rank > 0 && rank <= RANK_KING) {
    return card->rank == (char)rank;
  }
  return rank == RANK_ANY;
}

int check_next_suit(Card *card, Card *previous, GameRuleSuit suit) {
  if (check_first_suit(card, suit)) {
    return 1;
  }
  switch (suit) {
    case SUIT_SAME:
      return card->suit == previous->suit;
    case SUIT_SAME_COLOR:
      return card->suit & RED == previous->suit & RED;
    case SUIT_DIFF:
      return card->suit != previous->suit;
    case SUIT_DIFF_COLOR:
      return card->suit & RED != previous->suit & RED;
  }
  return 0;
}

int check_next_rank(Card *card, Card *previous, GameRuleRank rank) {
  if (check_first_rank(card, rank)) {
    return 1;
  }
  switch (rank) {
    case RANK_SAME:
      return card->rank == previous->rank;
    case RANK_DOWN:
      return card->rank == previous->rank - 1;
    case RANK_UP:
      return card->rank == previous->rank + 1;
    case RANK_UP_DOWN:
      return card->rank == previous->rank - 1 || card->rank == previous->rank + 1;
    case RANK_LOWER:
      return card->rank < previous->rank;
    case RANK_HIGHER:
      return card->rank > previous->rank;
  }
  return 0;
}

int check_stack(Card *stack, GameRuleSuit suit, GameRuleRank rank) {
  if (!stack) {
    return 1;
  }
  return check_next_suit(stack, stack->prev, suit) &&
    check_next_rank(stack, stack->prev, rank) && check_stack(stack->next, suit, rank);
}

int legal_move_stack(Pile *dest, Card *src, Pile *src_pile) {
  if (get_bottom(src) == get_bottom(dest->stack)) {
    return 0;
  }
  if (dest->rule->from != RULE_ANY && dest->rule->from != src_pile->rule->type) {
    return 0;
  }
  if (dest->rule->move_group == MOVE_ONE && src->next) {
    return 0;
  }
  if (dest->rule->move_group == MOVE_GROUP && src->next && !check_stack(src->next, dest->rule->next_suit, dest->rule->next_rank)) {
    return 0;
  }
  if (dest->stack->next) {
    Card *top = get_top(dest->stack);
    if (!top->up) {
      return 0;
    }
    if (!check_next_suit(src, top, dest->rule->next_suit) || !check_next_rank(src, top, dest->rule->next_rank)) {
      return 0;
    }
    top->next = src;
    if (src->prev) src->prev->next = NULL;
    src->prev = top;
    return 1;
  } else {
    if (!check_first_suit(src, dest->rule->first_suit) || !check_first_rank(src, dest->rule->first_rank)) {
      return 0;
    }
    dest->stack->next = src;
    if (src->prev) src->prev->next = NULL;
    src->prev = dest->stack;
    return 1;
  }
}

int move_to_waste(Card *card, Pile *stock, Pile *piles) {
  for (Pile *dest = piles; dest; dest = dest->next) {
    if (dest->rule->type == RULE_WASTE) {
      if (legal_move_stack(dest, card, stock)) {
        card->up = 1;
        return 1;
      }
    }
  }
  return 0;
}

int redeal(Pile *stock, Pile *piles) {
  if (stock->redeals < stock->rule->redeals) {
    stock->redeals++;
    for (Pile *src = piles; src; src = src->next) {
      if (src->rule->type == RULE_WASTE) {
        Card *src_card = get_top(src->stack);
        while (!(src_card->suit & BOTTOM)) {
          Card *prev = src_card->prev;
          move_stack(stock->stack, src_card);
          src_card = prev;
        }
      }
    }
    return 1;
  }
  return 0;
}

int auto_move_to_foundation(Pile *piles) {
  for (Pile *src = piles; src; src = src->next) {
    if (src->rule->type != RULE_FOUNDATION && src->rule->type != RULE_STOCK) {
      Card *src_card = get_top(src->stack);
      if (!(src_card->suit & BOTTOM)) {
        if (!src_card->up) {
          src_card->up = 1;
          return 1;
        } else {
          for (Pile *dest = piles; dest; dest = dest->next) {
            if (dest->rule->type == RULE_FOUNDATION) {
              if (legal_move_stack(dest, src_card, src)) {
                return 1;
              }
            }
          }
        }
      }
    }
  }
  return 0;
}
