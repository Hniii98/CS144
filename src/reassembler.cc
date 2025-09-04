#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert(uint64_t first_index, std::string data, bool is_last_substring) {
  uint64_t window_left  = first_unassembled_index();
  uint64_t window_right = window_left + available_capacity();

  // Meet the last interval, record eof_index_ in first time and ignore later
  if (is_last_substring && !have_eof_) {
      eof_index_ = first_index + data.size();
      have_eof_  = true;
  }

  uint64_t clip_left  = std::max(first_index, window_left);
  uint64_t clip_right = std::min(first_index + data.size(), window_right);

  if (clip_left < clip_right) { // overlap exist in writing window
      uint64_t pos = clip_left - first_index;
      uint64_t len = clip_right - clip_left;
      std::string slice = data.substr(pos, len);

      if (clip_left == window_left) {
          output_.writer().push(slice);
          normalize_buffer();  // normalize buffer          
          drain(); // drain from buffer

      } else {
          buffer_it(clip_left, std::move(slice));
      }
  }

  

  //  Check if received all data
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

void Reassembler::buffer_it(uint64_t index, std::string data) {
  if (data.empty()) return;

  uint64_t L = index;
  uint64_t R = index + data.size();      

  // Make sure when it->first < L must handled (it's first index less than index, but it may reach to overlap data)
  auto it = internal_buffer_.upper_bound(L);
  if (it != internal_buffer_.begin()) --it;

  while (it != internal_buffer_.end()) {
    uint64_t s = it->first;
    uint64_t e = s + it->second.size();

    if (e < L) {  // Previous one are no overlap, skip. otherwise go to handle left and right slice
      ++it;
      continue;
    }

    if (s > R) { // all overlap handled over, break loop      
      break;
    }

    // Every buffered interval, we consider it's front and tail.
    // So we don't have to clarify it to many situation.

    if (s < L) { // prefix
      data = it->second.substr(0, L - s) + data;
      L = s;
    }
    if (e > R) { // suffix
      data += it->second.substr(R - s);
      R = e;
    }

    // remove old pair
    it = internal_buffer_.erase(it);
  }

  internal_buffer_.emplace(L, std::move(data));
}


void Reassembler::normalize_buffer() {
  if (internal_buffer_.empty()) return;

  const uint64_t wanted = first_unassembled_index();
  auto it = internal_buffer_.begin();

 
  while (it != internal_buffer_.end() && it->first < wanted) {
    const uint64_t start = it->first;
    const std::string &s = it->second;
    const uint64_t end = start + s.size();

    if (end <= wanted) {
      it = internal_buffer_.erase(it);
    } else {
      const size_t off = static_cast<size_t>(wanted - start);
      std::string tail = s.substr(off);
      it = internal_buffer_.erase(it);
      internal_buffer_.emplace(wanted, std::move(tail));
      break;
    }
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
