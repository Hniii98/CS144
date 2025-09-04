#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert(uint64_t first_index, std::string data, bool is_last_substring) {
    uint64_t window_left  = first_unassembled_index();
    uint64_t window_right = window_left + available_capacity();

    if (is_last_substring) {
        eof_index_ = first_index + data.size();
        have_eof_  = true;
    }

    uint64_t clip_left  = std::max(first_index, window_left);
    uint64_t clip_right = std::min(first_index + data.size(), window_right);

    if (clip_left < clip_right) {
        uint64_t pos = clip_left - first_index;
        uint64_t len = clip_right - clip_left;
        std::string slice = data.substr(pos, len);

        if (clip_left == window_left) {
            output_.writer().push(slice);
            drain();
        } else {
            buffer_it(clip_left, std::move(slice));
        }
    }

    // 2. 检查是否已经接收完所有数据
    if (have_eof_ && first_unassembled_index() == eof_index_) {
        output_.writer().close();
    }
}


void Reassembler::drain() {
  if(internal_buffer_.empty()) return; // buffer empty, return

  uint64_t wanted = first_unassembled_index();
  uint64_t cap = available_capacity();
  auto it = internal_buffer_.find(wanted);

  while (it != internal_buffer_.end() && cap){
    uint64_t m = min(cap, it->second.size());

    // Push available lenth of string
    output_.writer().push(it->second.substr(0, m));
  
    // Check if remove all it->second string
    bool push_full_string = (m ==  it->second.size());


    if(push_full_string) { 
      internal_buffer_.erase(it); // remove directly
    } else {
      uint64_t new_index = it->first + m;
      string tail = it->second.substr(m);
      internal_buffer_.erase(it);
      internal_buffer_.emplace(new_index, std::move(tail)); // write tail to buffer
    }

   
    // Update status
    wanted = first_unassembled_index();
    cap = available_capacity();
    it = internal_buffer_.find(wanted);
  }  
}


void Reassembler::buffer_it(uint64_t index, string data) {
  if(internal_buffer_.empty()) { // First time buffer, insert directly
    internal_buffer_.emplace(index, data);
    return;
  }

  // At least one pair in internal_buffer_
  auto greater_equal = internal_buffer_.lower_bound(index); 
  auto less = prev(greater_equal);


  // Calculate interval not overlap
  uint64_t clip_left = (less == internal_buffer_.begin()) ?
                        index :
                        max(less->first + less->second.size() - 1, index);
  
  uint64_t clip_right = (greater_equal == internal_buffer_.end()) ?
                        index + data.size() :
                        min(index + data.size(), greater_equal->first);

  string slice = data.substr(clip_left - index, clip_right - clip_left + 1);


  // Check if can apeend to less node
  bool append_to_left = (less != internal_buffer_.begin()) &&
                        ((less->first + less->second.size()) >= index);


  if(append_to_left) {
    less->second.append(slice);
  } else {
    internal_buffer_.emplace(index, slice);
  }
  
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const {
  uint64_t total = 0;
  for (auto & [index, seg] : internal_buffer_) {
      total += seg.size();
  }
  return total;
}
