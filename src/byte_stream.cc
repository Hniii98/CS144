#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : 
  capacity_( capacity ), 
  have_pushed_ ( 0 ),
  have_popped_( 0 )
  {}

void Writer::push( string data )
{
  uint64_t push_availabe = std::min(data.size(), available_capacity());
  stream_.append(data.begin(), data.begin() + push_availabe);
  have_pushed_ += push_availabe;
}

void Writer::close()
{
  closed_ = true; 
}

bool Writer::is_closed() const
{
  return closed_; // Return stream_ status.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - stream_.size(); // Remaining space in stream_.
}

uint64_t Writer::bytes_pushed() const
{
  return have_pushed_;
}

string_view Reader::peek() const
{
  return string_view(stream_.data(), stream_.size());
}

void Reader::pop( uint64_t len )
{
  uint64_t pop_available = std::min(len,
    static_cast<uint64_t>(stream_.size()));

  stream_.erase(stream_.begin(), stream_.begin() + pop_available);
  have_popped_ += pop_available;

}

bool Reader::is_finished() const
{
  return closed_ and (have_popped_ == have_pushed_);
  
}

uint64_t Reader::bytes_buffered() const
{
  return stream_.size();
}

uint64_t Reader::bytes_popped() const
{
  return have_popped_;
}

