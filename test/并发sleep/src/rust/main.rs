use std::time::{Duration, Instant};
use std::thread::sleep;
use std::thread;
use std::sync::{Arc, Mutex};
fn main() {
    let now = Instant::now();
    let cnt = Arc::new(Mutex::new(0));

    for i in 0..1000000{
        let data=cnt.clone();
        thread::spawn(move|| {
        sleep(Duration::from_millis(100));
        let mut data = data.lock().unwrap();
        *data += 1;
        });
}
   {let r:&u32=&cnt.lock().unwrap();
//   println!("stcnt:{}",&r);
}
    while true{
   {let r:&u32=&cnt.lock().unwrap();
//   println!("cnt:{}",&r);
    if r>=&1000000{break;}}
    sleep(Duration::from_millis(10)); 
    }
    let new_now = Instant::now();
    println!("total sec:{:?},nano:{:?},{:?}", new_now.duration_since(now).as_secs(),new_now.duration_since(now).subsec_nanos(),&cnt);
}

