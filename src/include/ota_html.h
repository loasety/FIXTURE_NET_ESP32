#ifndef OTA_HTML_H
#define OTA_HTML_H

// 使用 C++11 raw string literal (R"rawliteral(...)rawliteral")，
// 这样在 HTML 内部无需转义任何引号和换行符，可以直接从 ota.html 复制粘贴进行修改
const char* serverIndex = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='utf-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>治具固件升级</title>
  <style>
    body { 
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; 
      text-align: center; 
      margin-top: 50px; 
      background: #f8f9fa; 
      color: #333; 
    }
    h2 { 
      color: #1a1f2c; 
      font-weight: 600;
      letter-spacing: 1px;
      margin-bottom: 40px;
    }
    .container { 
      background: #ffffff; 
      padding: 40px; 
      display: inline-block; 
      max-width: 450px; 
      width: 90%; 
      border: 1px solid #eaeaea;
      box-shadow: 0 4px 20px rgba(0,0,0,0.03);
      border-radius: 12px;
    }
    .file-container {
      text-align: left;
      margin-bottom: 25px;
    }
    .file-input { 
      width: 100%; 
      color: #a7a2a2; 
      font-size: 14px;
    }
    .btn { 
      background: #333436; 
      color: white; 
      padding: 14px 24px; 
      border: none; 
      border-radius: 8px; 
      font-size: 16px; 
      font-weight: 600; 
      cursor: pointer; 
      width: 100%; 
      transition: background 0.2s ease; 
    }
    .btn:hover { 
      background: #2f3336; 
    }
    .divider {
      height: 1px;
      margin: 10px 0;
    }
    
    #log_box {
      text-align: left;
      font-family: Consolas, Monaco, "Courier New", monospace;
      font-size: 13px;
      color: #666;
      line-height: 1.8;
      display: none;
    }
    .log-item {
      margin-bottom: 12px;
      animation: fadeIn 0.3s ease-out;
    }
    .log-item.bold {
      font-weight: bold;
      color: #111;
    }
    .subtext {
      font-size: 11px;
      color: #999;
      margin-left: 48px;
      margin-top: -2px;
      display: block;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; 
    }
    
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(-5px); }
      to { opacity: 1; transform: translateY(0); }
    }
  </style>
</head>
<body>
  <div class='container'>
    <h2>治具固件升级</h2>
    
    <div style="color: #666; font-size: 13px; margin-top: -25px; margin-bottom: 30px; letter-spacing: 0.5px;">
      当前设备固件版本：<strong style="color: #111; font-family: monospace; font-size: 14px;">{{CURRENT_VERSION}}</strong>
    </div>

    <form id='upload_form'>
      <div class="file-container">
        <input type='file' name='update' class='file-input' id='file' accept='.bin' required>
      </div>
      <button type='submit' class='btn'>刷入新固件</button>
    </form>
    
    <div class="divider"></div>
    
    <div id='log_box'></div>
  </div>

  <script>
    function addLog(step, mainText, subText, isBold, id) {
      var box = document.getElementById('log_box');
      box.style.display = 'block';
      
      var item;
      if (id && document.getElementById(id)) {
        item = document.getElementById(id);
      } else {
        item = document.createElement('div');
        item.className = 'log-item' + (isBold ? ' bold' : '');
        if (id) item.id = id;
        box.appendChild(item);
      }
      
      var html = '[' + step + '/4] ' + mainText;
      if (subText) {
        html += '<span class="subtext">' + subText + '</span>';
      }
      item.innerHTML = html;
      box.scrollTop = box.scrollHeight;
    }

    document.getElementById('upload_form').addEventListener('submit', function(e) {
      e.preventDefault();
      var file = document.getElementById('file').files[0];
      if(!file) {
        alert("请选择固件文件");
        return;
      }
      
      var logBox = document.getElementById('log_box');
      logBox.innerHTML = ''; 
      
      addLog('1', '正在准备发送固件...', null, false);
      
      var formData = new FormData();
      formData.append('update', file, file.name);
      
      var request = new XMLHttpRequest();
      request.open('POST', '/update');
      
      request.upload.addEventListener('progress', function(e) {
        if(e.lengthComputable) {
          var percent = Math.round((e.loaded / e.total) * 100);
          var loadedKB = (e.loaded / 1024).toFixed(1);
          var totalKB = (e.total / 1024).toFixed(1);
          
          if(percent < 100) {
            addLog('2', '正在传输: ' + percent + '% (' + loadedKB + 'KB / ' + totalKB + 'KB)', null, false, 'log_progress');
          } else {
            addLog('2', '正在传输: 100% (' + totalKB + 'KB / ' + totalKB + 'KB)', null, false, 'log_progress');
            addLog('3', '烧录中...', '需5-10秒，请勿断开连接或断电。', false);
          }
        }
      });
      
      request.onreadystatechange = function() {
        if (request.readyState === 4) {
          if (request.status === 200 && request.responseText === 'OK') {
            addLog('4', '升级成功，正在重启，3s后请手动复位...', null, true);
          } else {
            addLog('4', '固件升级失败！请重试。', null, true);
          }
        }
      };
      
      request.send(formData);
    });
  </script>
</body>
</html>
)rawliteral";

#endif
