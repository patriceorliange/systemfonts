#include <limits.h>
#include <algorithm>
#include "string_shape.h"

UTF_UCS FreetypeShaper::utf_converter = UTF_UCS();
std::vector<u_int32_t> FreetypeShaper::glyph_uc = {};
std::vector<unsigned int> FreetypeShaper::glyph_id = {};
std::vector<unsigned int> FreetypeShaper::string_id = {};
std::vector<long> FreetypeShaper::x_pos = {};
std::vector<long> FreetypeShaper::y_pos = {};

bool FreetypeShaper::shape_string(const char* string, const char* fontfile, 
                                  int index, double size, double res, double lineheight,
                                  int align, double hjust, double vjust) {
  reset();
  
  FreetypeCache& cache = get_font_cache();
  bool success = cache.load_font(fontfile, index, size, res);
  if (!success) {
    error_code = cache.error_code;
    return false;
  }
  
  int n_glyphs = 0;
  u_int32_t* glyphs = utf_converter.convert(string, n_glyphs);
  
  if (n_glyphs == 0) return true;
  
  glyph_uc.reserve(n_glyphs);
  glyph_id.reserve(n_glyphs);
  string_id.reserve(n_glyphs);
  x_pos.reserve(n_glyphs);
  y_pos.reserve(n_glyphs);
  
  cur_res = res;
  cur_lineheight = lineheight;
  cur_align = align;
  cur_hjust = hjust;
  cur_vjust = vjust;
  cur_full_lineheight = cache.cur_lineheight() * lineheight;
  
  ascend = cache.cur_ascender();
  descend = cache.cur_descender();
  top = ascend;
  top_border = top;
  bottom = descend;
  top_bearing = LONG_MAX;
  bottom_bearing = LONG_MAX;
  
  success = shape_glyphs(glyphs, n_glyphs, cache);
  return success;
};

bool FreetypeShaper::add_string(const char* string, const char* fontfile, 
                                int index, double size) {
  cur_string++;
  int n_glyphs = 0;
  u_int32_t* glyphs = utf_converter.convert(string, n_glyphs);
  
  if (n_glyphs == 0) return true;
  
  FreetypeCache& cache = get_font_cache();
  bool success = cache.load_font(fontfile, index, size, cur_res);
  if (!success) {
    error_code = cache.error_code;
    return false;
  }
  ascend = cache.cur_ascender();
  descend = cache.cur_descender();
  top = top < ascend ? ascend : top;
  top_border = top;
  cur_full_lineheight = cur_full_lineheight < cache.cur_lineheight() * cur_lineheight ? cache.cur_lineheight() * cur_lineheight : cur_full_lineheight;
  if (firstline) {
    descend = descend < cache.cur_descender() ? cache.cur_descender() : descend;
  }
  kern = false;
  line_right_bear.pop_back();
  success = shape_glyphs(glyphs, n_glyphs, cache);
  return success;
}
  
bool FreetypeShaper::finish_string() {
  line_width.push_back(pen_x);
  bottom -= cur_full_lineheight;
  pen_y = firstline ? 0 : pen_y - cur_full_lineheight;
  for (; first_glyph < x_pos.size(); first_glyph++) {
    y_pos.push_back(pen_y);
  }
  
  height = top_border - bottom;
  int max_width_ind = std::max_element(line_width.begin(), line_width.end()) - line_width.begin();
  width = line_width[max_width_ind];
  if (cur_align != 0) {
    for (unsigned int i = 0; i < x_pos.size(); ++i) {
      int index = line_id[i];
      int lwd = line_width[index];
      x_pos[i] = cur_align == 1 ? x_pos[i] + width/2 - lwd/2 : x_pos[i] + width - lwd;
    }
  }
  left_bearing = cur_align == 0 ? *std::min_element(line_left_bear.begin(), line_left_bear.end()) : line_left_bear[max_width_ind];
  right_bearing = cur_align == 2 ? *std::min_element(line_right_bear.begin(), line_right_bear.end()) : line_right_bear[max_width_ind];
  if (cur_hjust != 0.0) {
    left_border = - cur_hjust * width;
    pen_x += left_border;
    for (unsigned int i = 0; i < x_pos.size(); ++i) {
      x_pos[i] += left_border;
    }
  }
  long just_height = ascend - pen_y;
  top_border += - pen_y - cur_vjust * just_height;
  pen_y += - pen_y - cur_vjust * just_height;
  for (unsigned int i = 0; i < x_pos.size(); ++i) {
    y_pos[i] += - pen_y - cur_vjust * just_height;
  }
  return true;
}

bool FreetypeShaper::single_line_width(const char* string, const char* fontfile, 
                                       int index, double size, double res, 
                                       bool include_bearing, long& width) {
  long x = 0;
  long y = 0;
  long left_bear = 0;
  int error_c;
  GlyphInfo metrics;
  
  int n_glyphs = 0;
  u_int32_t* glyphs = utf_converter.convert(string, n_glyphs);
  
  if (n_glyphs == 0) {
    width = x;
    return true;
  }
  
  FreetypeCache& cache = get_font_cache();
  bool success = cache.load_font(fontfile, index, size, res);
  if (!success) {
    error_code = cache.error_code;
    return false;
  }
  
  for (int i = 0; i < n_glyphs; ++i) {
    metrics = cache.cached_glyph_info(glyphs[i], error_c);
    if (error_c != 0) {
      error_code = error_c;
      return false;
    }
    if (i != 0) {
      success = cache.apply_kerning(glyphs[i - 1], glyphs[i], x, y);
      if (!success) {
        error_code = cache.error_code;
        return false;
      }
    } else {
      left_bear = metrics.x_bearing;
    }
    x += metrics.x_advance;
  }
  
  if (!include_bearing) {
    x -= left_bear;
    x -= metrics.x_advance - metrics.bbox[1];
  }
  width = x;
  return true;
}

void FreetypeShaper::reset() {
  glyph_uc.clear();
  glyph_id.clear();
  string_id.clear();
  x_pos.clear();
  x_pos.clear();
  y_pos.clear();
  line_left_bear.clear();
  line_right_bear.clear();
  line_width.clear();
  line_id.clear();
  
  pen_x = 0;
  pen_y = 0;
  
  top = 0;
  bottom = 0;
  ascend = 0;
  descend = 0;
  
  left_bearing = 0;
  right_bearing = 0;
  width = 0;
  height = 0;
  top_border = 0;
  left_border = 0;
  
  kern = false;
  firstline = true;
  cur_line = 0;
  first_glyph = 0;
  cur_string = 0;
}

bool FreetypeShaper::shape_glyphs(u_int32_t* glyphs, int n_glyphs, FreetypeCache& cache) {
  if (n_glyphs == 0) return true;
  int error_c; 
  bool success;
  GlyphInfo old_metrics = cache.cached_glyph_info(glyphs[0], error_c);
  if (error_c != 0) {
    error_code = error_c;
    return false;
  }
  GlyphInfo metrics = old_metrics;
  
  if (firstline) {
    line_left_bear.push_back(metrics.bbox[0]);
  }
  
  for (int i = 0; i < n_glyphs; ++i) {
    bool linebreak = glyphs[i] == 10;
    bool last = i == n_glyphs - 1;
    if (kern && !linebreak) {
      success = cache.apply_kerning(glyphs[i - 1], glyphs[i], pen_x, pen_y);
      if (!success) {
        error_code = cache.error_code;
        return false;
      }
    }
    glyph_uc.push_back(glyphs[i]);
    glyph_id.push_back(metrics.index);
    string_id.push_back(cur_string);
    x_pos.push_back(pen_x);
    line_id.push_back(cur_line);
    if (firstline) {
      long tb = ascend - metrics.bbox[3];
      top_bearing = top_bearing < tb ? top_bearing : tb;
    }
    long bb = metrics.bbox[2] - descend;
    bottom_bearing = bottom_bearing < bb ? bottom_bearing : bb;
    if (linebreak) { // linebreak
      line_right_bear.push_back(old_metrics.x_advance - old_metrics.bbox[1]);
      line_width.push_back(pen_x);
      pen_x = 0;
      pen_y = firstline ? 0 : pen_y - cur_full_lineheight;
      bottom -= cur_full_lineheight;
      cur_full_lineheight = cache.cur_lineheight() * cur_lineheight;
      for (; first_glyph < x_pos.size(); first_glyph++) {
        y_pos.push_back(pen_y);
      }
      bottom_bearing = last ? 0 : LONG_MAX;
      kern = false;
      firstline = false;
      cur_line++;
    } else {
      pen_x += metrics.x_advance;
      kern = true;
    }
    if (!last) {
      old_metrics = metrics;
      metrics = cache.cached_glyph_info(glyphs[i + 1], error_c);
      if (error_c != 0) {
        error_code = error_c;
        return false;
      }
      if (linebreak) {
        line_left_bear.push_back(metrics.bbox[0]);
      }
    }
  }
  line_right_bear.push_back(metrics.x_advance - metrics.bbox[1]);
  return true;
}
