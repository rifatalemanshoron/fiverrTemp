const ws = new WebSocket('wss://pedantic-keldysh.85-215-172-49.plesk.page/');
ws.onopen = ()=>console.log('open'); ws.onmessage = e=>console.log('msg', e.data);
