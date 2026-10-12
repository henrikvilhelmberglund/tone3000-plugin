// The picture view: a block's picture over the whole window, nothing else
// on screen ("i" with the block's card open). The picture fills the width,
// centred, its top and bottom cut off where it is taller than the window
// (bands above and below where it is wider). Behind it, the colour of its
// edges: black for a transparent or black-edged picture, white for a white
// one, a photo's own edge tone, so the bands and see-through parts read as
// the picture's. A close button shows
// while the mouse moves and goes with the cursor when it rests; "i" again,
// Escape or the button leaves. The view follows the block: a model switched with
// the arrow keys (the card is still open under it) brings that model's
// picture, and the view closes when the block goes.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <string>

#include "core/Theme.h"
#include "services/Services.h"
#include "widgets/IconButton.h"

namespace t3k::ui {

class PictureView : public juce::Component, private ChainStore::Listener, private juce::Timer {
public:
  // The longest side asked of the image loader: a 1024-wide window on a 2x
  // display, with a little zoom headroom.
  static constexpr int kMaxSide = 2400;
  static constexpr int kCloseBox = 28, kCloseGlyph = 20, kCloseInset = 16;
  static constexpr int kIdleMs = 2000;

  PictureView(Services& services, std::string blockId);
  ~PictureView() override;

  std::function<void()> onClose;

  const std::string& blockId() const { return blockId_; }
  // The picture shown (null while it loads, or for a block without one).
  const juce::Image& image() const { return image_; }
  // Where the picture lands in the view: the full width, centred.
  static juce::Rectangle<float> placement(juce::Rectangle<float> box, int imageW, int imageH);
  // The average of the picture's outermost pixels, each over black (a
  // transparent one counts as black): what the view shows behind it.
  static juce::Colour edgeColour(const juce::Image& image);
  juce::Colour backdrop() const { return backdrop_; }

  void paint(juce::Graphics& g) override;
  void resized() override;
  void mouseMove(const juce::MouseEvent&) override { wake(); }
  void mouseDrag(const juce::MouseEvent&) override { wake(); }

private:
  void chainChanged(const ChainState& state) override;
  void timerCallback() override;
  // The mouse moved: the close button and the cursor show, for a while.
  void wake();
  void rest();

  Services& services_;
  std::string blockId_;
  juce::String url_, title_;
  ImageLoader::Request request_;
  juce::Image image_;
  juce::Colour backdrop_ = theme::kBlack;
  bool loading_ = false;
  IconButton close_{Icon::X, kCloseBox, kCloseGlyph};
};

}  // namespace t3k::ui
