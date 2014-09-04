#include "sample.h"
#include <string.h>
#include <stdio.h>

namespace app
{
  using namespace pge;

  namespace sample_keyboard
  {
    u64 ki1, ki2, ki3, ki4;

    char p[4096];
    const u32 max_char = 128;
    char tmp[max_char];
    f32 spacing = 40.f;

    void init()
    {
      ki1 = world::spawn_text(global_game_world, global_font_name, NULL, TEXT_ALIGN_LEFT, IDENTITY_TRANSLATION, IDENTITY_ROTATION);
      ki2 = world::spawn_text(global_game_world, global_font_name, NULL, TEXT_ALIGN_LEFT, IDENTITY_TRANSLATION, IDENTITY_ROTATION);
      ki3 = world::spawn_text(global_game_world, global_font_name, NULL, TEXT_ALIGN_LEFT, IDENTITY_TRANSLATION, IDENTITY_ROTATION);
      ki4 = world::spawn_text(global_game_world, global_font_name, NULL, TEXT_ALIGN_LEFT, IDENTITY_TRANSLATION, IDENTITY_ROTATION);
      update(0.f);
    }

    void update(f64 dt)
    {
      (void)dt;
      if (dt && !keyboard::any_pressed() && !keyboard::any_released()) return;
      glm::vec2 tmp_size;

      strcpy(p, "\0");
      sprintf(tmp, "0: %s", keyboard::button(KEYBOARD_KEY_0) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " 1: %s", keyboard::button(KEYBOARD_KEY_1) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " 2: %s", keyboard::button(KEYBOARD_KEY_2) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\n3: %s", keyboard::button(KEYBOARD_KEY_3) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " 4: %s", keyboard::button(KEYBOARD_KEY_4) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " 5: %s", keyboard::button(KEYBOARD_KEY_5) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\n6: %s", keyboard::button(KEYBOARD_KEY_6) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " 7: %s", keyboard::button(KEYBOARD_KEY_7) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " 8: %s", keyboard::button(KEYBOARD_KEY_8) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\n9: %s", keyboard::button(KEYBOARD_KEY_9) ? L"X" : L"_"); strcat(p, tmp);

      sprintf(tmp, "\n\nA: %s", keyboard::button(KEYBOARD_KEY_A) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " B: %s", keyboard::button(KEYBOARD_KEY_B) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " C: %s", keyboard::button(KEYBOARD_KEY_C) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nD: %s", keyboard::button(KEYBOARD_KEY_D) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " E: %s", keyboard::button(KEYBOARD_KEY_E) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F: %s", keyboard::button(KEYBOARD_KEY_F) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nG: %s", keyboard::button(KEYBOARD_KEY_G) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " H: %s", keyboard::button(KEYBOARD_KEY_H) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " I: %s", keyboard::button(KEYBOARD_KEY_I) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nJ: %s", keyboard::button(KEYBOARD_KEY_J) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " K: %s", keyboard::button(KEYBOARD_KEY_K) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " L: %s", keyboard::button(KEYBOARD_KEY_L) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nM: %s", keyboard::button(KEYBOARD_KEY_M) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " N: %s", keyboard::button(KEYBOARD_KEY_N) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " O: %s", keyboard::button(KEYBOARD_KEY_O) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nP: %s", keyboard::button(KEYBOARD_KEY_P) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " Q: %s", keyboard::button(KEYBOARD_KEY_Q) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " R: %s", keyboard::button(KEYBOARD_KEY_R) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nS: %s", keyboard::button(KEYBOARD_KEY_S) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " T: %s", keyboard::button(KEYBOARD_KEY_T) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " U: %s", keyboard::button(KEYBOARD_KEY_U) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nV: %s", keyboard::button(KEYBOARD_KEY_V) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " W: %s", keyboard::button(KEYBOARD_KEY_W) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " X: %s", keyboard::button(KEYBOARD_KEY_X) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nY: %s", keyboard::button(KEYBOARD_KEY_Y) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " Z: %s", keyboard::button(KEYBOARD_KEY_Z) ? L"X" : L"_"); strcat(p, tmp);

      sprintf(tmp, "\n\nRIGHT: %s", keyboard::button(KEYBOARD_KEY_RIGHT) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " LEFT: %s", keyboard::button(KEYBOARD_KEY_LEFT) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nDOWN:  %s", keyboard::button(KEYBOARD_KEY_DOWN) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " UP:   %s", keyboard::button(KEYBOARD_KEY_UP) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\n\nF1:  %s", keyboard::button(KEYBOARD_KEY_F1) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F2:  %s", keyboard::button(KEYBOARD_KEY_F2) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F3:  %s", keyboard::button(KEYBOARD_KEY_F3) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nF4:  %s", keyboard::button(KEYBOARD_KEY_F4) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F5:  %s", keyboard::button(KEYBOARD_KEY_F5) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F6:  %s", keyboard::button(KEYBOARD_KEY_F6) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nF7:  %s", keyboard::button(KEYBOARD_KEY_F7) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F8:  %s", keyboard::button(KEYBOARD_KEY_F8) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F9:  %s", keyboard::button(KEYBOARD_KEY_F9) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nF10: %s", keyboard::button(KEYBOARD_KEY_F10) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F11: %s", keyboard::button(KEYBOARD_KEY_F11) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F12: %s", keyboard::button(KEYBOARD_KEY_F12) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nF13: %s", keyboard::button(KEYBOARD_KEY_F13) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F14: %s", keyboard::button(KEYBOARD_KEY_F14) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F15: %s", keyboard::button(KEYBOARD_KEY_F15) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nF16: %s", keyboard::button(KEYBOARD_KEY_F16) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F17: %s", keyboard::button(KEYBOARD_KEY_F17) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F18: %s", keyboard::button(KEYBOARD_KEY_F18) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nF19: %s", keyboard::button(KEYBOARD_KEY_F19) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F20: %s", keyboard::button(KEYBOARD_KEY_F20) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F21: %s", keyboard::button(KEYBOARD_KEY_F21) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nF22: %s", keyboard::button(KEYBOARD_KEY_F22) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F23: %s", keyboard::button(KEYBOARD_KEY_F23) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " F24: %s", keyboard::button(KEYBOARD_KEY_F24) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nF25: %s", keyboard::button(KEYBOARD_KEY_F25) ? L"X" : L"_"); strcat(p, tmp);

      glm::vec3 position_p1(global_screen.width*-.5f, global_screen.h2, 0.f);
      text::set_string(global_game_world, ki1, p);
      text::set_local_position(global_game_world, ki1, position_p1);

      strcpy(p, "\0");
      sprintf(tmp, "SPACE:         %s", keyboard::button(KEYBOARD_KEY_SPACE) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nAPOSTROPHE:    %s", keyboard::button(KEYBOARD_KEY_APOSTROPHE) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nCOMMA:         %s", keyboard::button(KEYBOARD_KEY_COMMA) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nMINUS:         %s", keyboard::button(KEYBOARD_KEY_MINUS) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nPERIOD:        %s", keyboard::button(KEYBOARD_KEY_PERIOD) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nSLASH:         %s", keyboard::button(KEYBOARD_KEY_SLASH) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nSEMICOLON:     %s", keyboard::button(KEYBOARD_KEY_SEMICOLON) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nEQUAL:         %s", keyboard::button(KEYBOARD_KEY_EQUAL) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nLEFT_BRACKET:  %s", keyboard::button(KEYBOARD_KEY_LEFT_BRACKET) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nBACKSLASH:     %s", keyboard::button(KEYBOARD_KEY_BACKSLASH) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nRIGHT_BRACKET: %s", keyboard::button(KEYBOARD_KEY_RIGHT_BRACKET) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nGRAVE_ACCENT:  %s", keyboard::button(KEYBOARD_KEY_GRAVE_ACCENT) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nWORLD_1:       %s", keyboard::button(KEYBOARD_KEY_WORLD_1) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nWORLD_2:       %s", keyboard::button(KEYBOARD_KEY_WORLD_2) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nESCAPE:        %s", keyboard::button(KEYBOARD_KEY_ESCAPE) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nENTER:         %s", keyboard::button(KEYBOARD_KEY_ENTER) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nTAB:           %s", keyboard::button(KEYBOARD_KEY_TAB) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nBACKSPACE:     %s", keyboard::button(KEYBOARD_KEY_BACKSPACE) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nINSERT:        %s", keyboard::button(KEYBOARD_KEY_INSERT) ? L"X" : L"_"); strcat(p, tmp);

      text::get_size(global_game_world, ki1, tmp_size);
      glm::vec3 position_p2(position_p1.x + tmp_size.x + spacing, position_p1.y, 0.f);
      text::set_string(global_game_world, ki2, p);
      text::set_local_position(global_game_world, ki2, position_p2);

      strcpy(p, "\0");
      sprintf(tmp, "DELETE:        %s", keyboard::button(KEYBOARD_KEY_DELETE) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nPAGE_UP:       %s", keyboard::button(KEYBOARD_KEY_PAGE_UP) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nPAGE_DOWN:     %s", keyboard::button(KEYBOARD_KEY_PAGE_DOWN) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nHOME:          %s", keyboard::button(KEYBOARD_KEY_HOME) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nEND:           %s", keyboard::button(KEYBOARD_KEY_END) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nCAPS_LOCK:     %s", keyboard::button(KEYBOARD_KEY_CAPS_LOCK) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nSCROLL_LOCK:   %s", keyboard::button(KEYBOARD_KEY_SCROLL_LOCK) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nNUM_LOCK:      %s", keyboard::button(KEYBOARD_KEY_NUM_LOCK) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nPRINT_SCREEN:  %s", keyboard::button(KEYBOARD_KEY_PRINT_SCREEN) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nPAUSE:         %s", keyboard::button(KEYBOARD_KEY_PAUSE) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nLEFT_SHIFT:    %s", keyboard::button(KEYBOARD_KEY_LEFT_SHIFT) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nLEFT_CONTROL:  %s", keyboard::button(KEYBOARD_KEY_LEFT_CONTROL) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nLEFT_ALT:      %s", keyboard::button(KEYBOARD_KEY_LEFT_ALT) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nLEFT_SUPER:    %s", keyboard::button(KEYBOARD_KEY_LEFT_SUPER) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nRIGHT_SHIFT:   %s", keyboard::button(KEYBOARD_KEY_RIGHT_SHIFT) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nRIGHT_CONTROL: %s", keyboard::button(KEYBOARD_KEY_RIGHT_CONTROL) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nRIGHT_ALT:     %s", keyboard::button(KEYBOARD_KEY_RIGHT_ALT) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nRIGHT_SUPER:   %s", keyboard::button(KEYBOARD_KEY_RIGHT_SUPER) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nMENU:          %s", keyboard::button(KEYBOARD_KEY_MENU) ? L"X" : L"_"); strcat(p, tmp);

      text::get_size(global_game_world, ki2, tmp_size);
      glm::vec3 position_p3(position_p2.x + tmp_size.x + spacing, position_p2.y, 0.f);
      text::set_string(global_game_world, ki3, p);
      text::set_local_position(global_game_world, ki3, position_p3);

      strcpy(p, "\0");
      sprintf(tmp, "KP_0: %s", keyboard::button(KEYBOARD_KEY_KP_0) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " KP_1: %s", keyboard::button(KEYBOARD_KEY_KP_1) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " KP_2: %s", keyboard::button(KEYBOARD_KEY_KP_2) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nKP_3: %s", keyboard::button(KEYBOARD_KEY_KP_3) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " KP_4: %s", keyboard::button(KEYBOARD_KEY_KP_4) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " KP_5: %s", keyboard::button(KEYBOARD_KEY_KP_5) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nKP_6: %s", keyboard::button(KEYBOARD_KEY_KP_6) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " KP_7: %s", keyboard::button(KEYBOARD_KEY_KP_7) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " KP_8: %s", keyboard::button(KEYBOARD_KEY_KP_8) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nKP_9: %s", keyboard::button(KEYBOARD_KEY_KP_9) ? L"X" : L"_"); strcat(p, tmp);

      sprintf(tmp, "\n\nKP_DECIMAL:  %s", keyboard::button(KEYBOARD_KEY_KP_DECIMAL) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " KP_DIVIDE:   %s", keyboard::button(KEYBOARD_KEY_KP_DIVIDE) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " KP_MULTIPLY: %s", keyboard::button(KEYBOARD_KEY_KP_MULTIPLY) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nKP_SUBTRACT: %s", keyboard::button(KEYBOARD_KEY_KP_SUBTRACT) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " KP_ADD:      %s", keyboard::button(KEYBOARD_KEY_KP_ADD) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, " KP_ENTER:    %s", keyboard::button(KEYBOARD_KEY_KP_ENTER) ? L"X" : L"_"); strcat(p, tmp);
      sprintf(tmp, "\nKP_EQUAL:    %s", keyboard::button(KEYBOARD_KEY_KP_EQUAL) ? L"X" : L"_"); strcat(p, tmp);

      text::get_size(global_game_world, ki2, tmp_size);
      glm::vec3 position_p4(position_p2.x, position_p2.y - tmp_size.y - spacing, 0.f);
      text::set_string(global_game_world, ki4, p);
      text::set_local_position(global_game_world, ki4, position_p4);
    }

    void shutdown()
    {
      world::despawn_text(global_game_world, ki1);
      world::despawn_text(global_game_world, ki2);
      world::despawn_text(global_game_world, ki3);
      world::despawn_text(global_game_world, ki4);
    }

  }
}