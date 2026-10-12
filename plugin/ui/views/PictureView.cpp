#include "PictureView.h"

#include "core/Fonts.h"
#include "core/Help.h"
#include "core/Theme.h"

namespace t3k::ui {

PictureView::PictureView(Services& services, std::string blockId)
    : services_(services), blockId_(std::move(blockId)) {
  setOpaque(true);
  setWantsKeyboardFocus(false);
  close_.setName("Close picture");
  close_.setTitle("Close picture");
  close_.setHelpText(help::text(help::Key::closePicture));
  close_.onClick = [this] {
    if (onClose) onClose();
  };
  addChildComponent(close_);
  services_.chain.addListener(this);
  chainChanged(services_.chain.state());
  wake();
}

PictureView::~PictureView() { services_.chain.removeListener(this); }

juce::Rectangle<float> PictureView::placement(juce::Rectangle<float> box, int imageW, int imageH) {
  if (imageW <= 0 || imageH <= 0 || box.isEmpty()) return {};
  const float h = box.getWidth() * imageH / static_cast<float>(imageW);
  return {box.getX(), box.getCentreY() - h / 2, box.getWidth(), h};
}

juce::Colour PictureView::edgeColour(const juce::Image& image) {
  if (!image.isValid()) return theme::kBlack;
  const int w = image.getWidth(), h = image.getHeight();
  const juce::Image::BitmapData pixels(image, juce::Image::BitmapData::readOnly);
  double r = 0, g = 0, b = 0;
  int n = 0;
  const auto add = [&](int x, int y) {
    const auto c = pixels.getPixelColour(x, y);
    const double a = c.getFloatAlpha();
    r += c.getFloatRed() * a;
    g += c.getFloatGreen() * a;
    b += c.getFloatBlue() * a;
    ++n;
  };
  for (int x = 0; x < w; ++x) {
    add(x, 0);
    if (h > 1) add(x, h - 1);
  }
  for (int y = 1; y < h - 1; ++y) {
    add(0, y);
    if (w > 1) add(w - 1, y);
  }
  return juce::Colour::fromFloatRGBA(static_cast<float>(r / n), static_cast<float>(g / n),
                                     static_cast<float>(b / n), 1.0f);
}

void PictureView::chainChanged(const ChainState& state) {
  const auto* block = state.findBlock(blockId_);
  if (block == nullptr || !block->isTone()) {
    // The block went (removed, the chain replaced): nothing to show.
    juce::MessageManager::callAsync([safe = juce::Component::SafePointer<PictureView>(this)] {
      if (safe != nullptr && safe->onClose) safe->onClose();
    });
    return;
  }
  title_ = block->tone.title;
  if (block->tone.image == url_) return;
  url_ = block->tone.image;
  // The last picture stays up until the next one is in (no black flash
  // between two captures with pictures).
  loading_ = url_.isNotEmpty();
  if (!loading_) image_ = {};
  services_.images.loadWhole(url_, kMaxSide, request_, [this](const juce::Image& image) {
    loading_ = false;
    image_ = image;
    backdrop_ = edgeColour(image);
    // The close button stays readable on a light picture.
    close_.setColour(backdrop_.getPerceivedBrightness() > 0.6f ? std::optional<juce::Colour>(theme::kBlack)
                                                               : std::nullopt);
    repaint();
  });
  repaint();
}

void PictureView::paint(juce::Graphics& g) {
  g.fillAll(image_.isValid() ? backdrop_ : theme::kBlack);
  if (image_.isValid()) {
    const auto at = placement(getLocalBounds().toFloat(), image_.getWidth(), image_.getHeight());
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(image_, at);
    return;
  }
  if (loading_) return;  // a moment of black, not a flash of text
  g.setColour(theme::kMuted);
  g.setFont(Fonts::sans(15.0f));
  g.drawFittedText(title_.isNotEmpty() ? title_ + ": no picture" : "No picture", getLocalBounds().reduced(24),
                   juce::Justification::centred, 2);
}

void PictureView::resized() {
  close_.setBounds(getWidth() - kCloseInset - kCloseBox, kCloseInset, kCloseBox, kCloseBox);
}

void PictureView::wake() {
  close_.setVisible(true);
  setMouseCursor(juce::MouseCursor::NormalCursor);
  startTimer(kIdleMs);
}

void PictureView::rest() {
  stopTimer();
  // Not while the pointer is on the button: it would go from under it.
  if (close_.isMouseOver(true)) return startTimer(kIdleMs);
  close_.setVisible(false);
  setMouseCursor(juce::MouseCursor::NoCursor);
}

void PictureView::timerCallback() { rest(); }

}  // namespace t3k::ui
