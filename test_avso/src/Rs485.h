#ifndef RCT_RS485_H_  // ヘッダガード開始：二重インクルードを防止する
#define RCT_RS485_H_

#include <Kernel.h>   // Mbed OSのカーネル機能を提供するヘッダファイル
#include <mbed.h>     // Mbed OSの基本的な機能を提供するヘッダファイル

// RS-485通信を行うための構造体
struct Rs485 {
  // コンストラクタ：RS-485通信の初期化を行う
  Rs485(const PinName tx, const PinName rx, const int baud, const PinName de) 
    : bus_{PB_6, PA_10, baud}, de_{de} {
    bus_.set_blocking(0);  // シリアル通信を非ブロッキングモードに設定
  }

  // データを送信する関数
  void uart_transmit(const uint8_t *send, const int len) {
    de_ = 1;       // 送信モードに切り替える
    flush();       // 送信前に受信バッファをクリアする
    bus_.write(send, len);  // データを送信する
    wait_us(3);    // 送信完了後に少し待機する（3マイクロ秒）
    de_ = 0;       // 受信モードに戻す
  }

  // テンプレート版の送信関数：送信データのサイズを自動で計算して送信する
  template<int N>
  void uart_transmit(const uint8_t (&send)[N]) {
    uart_transmit(send, sizeof(send));
  }

  // データを受信する関数
  bool uart_receive(void *buf, const int len, const std::chrono::milliseconds timeout) {
    auto pre = Kernel::Clock::now();  // 受信開始時間を取得
    uint8_t *p = reinterpret_cast<uint8_t *>(buf);  // バッファポインタに変換
    const uint8_t *end = p + len;  // 受信終了位置を設定

    do {
      // 1バイト受信し、成功したら次のバイト位置へ移動
      if(bus_.read(p, 1) > 0 && ++p == end) {
        return (wait_ns(275), true);  // 受信完了後に275ナノ秒待機してtrueを返す
      }
    } while(Kernel::Clock::now() - pre < timeout);  // タイムアウトまでループ

    return false;  // タイムアウトに達したらfalseを返す
  }

  // テンプレート版の受信関数：受信バッファのサイズを自動で計算して受信する
  template<int N>
  bool uart_receive(uint8_t (&buf)[N], const std::chrono::milliseconds timeout) {
    return uart_receive(buf, sizeof(buf), timeout);
  }

 private:
  // 受信バッファをクリアする関数
  void flush() {
    uint8_t buf;
    while(bus_.read(&buf, 1) > 0) {}  // バッファが空になるまでデータを読み出して破棄
  }

  BufferedSerial bus_;  // シリアル通信を扱うためのMbedクラスインスタンス
  DigitalOut de_;       // 送受信の切り替えを制御するデジタル出力ピン
};

#endif  // RCT_RS485_H_  // ヘッダガード終了
