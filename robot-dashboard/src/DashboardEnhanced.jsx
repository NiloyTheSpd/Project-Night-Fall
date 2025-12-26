import React, { useState, useEffect, useRef } from 'react';
import {
  Wifi,
  WifiOff,
  Zap,
  Thermometer,
  Wind,
  Compass,
  Radio,
  Activity,
  AlertCircle,
  CheckCircle,
  Clock,
  Gauge
} from 'lucide-react';

const ESP32Dashboard = () => {
  const [esp32Data, setEsp32Data] = useState({
    back: {
      status: 'connecting',
      battery: 85,
      temperature: 42.5,
      signal: 85,
      uptime: 0,
      sensors: {
        frontDist: 0,
        rearDist: 0,
        gasLevel: 0,
        gasAlertLevel: false,
        navigationState: 'idle'
      },
      motors: {
        rearLeft: 0,
        rearRight: 0,
        isMoving: false
      },
      memoryUsage: 45,
      cpuLoad: 52
    },
    front: {
      status: 'connecting',
      battery: 92,
      temperature: 38.2,
      signal: 90,
      uptime: 0,
      motors: {
        motor1: 0,
        motor2: 0,
        motor3: 0,
        motor4: 0
      },
      memoryUsage: 38,
      cpuLoad: 28
    },
    camera: {
      status: 'connecting',
      battery: 88,
      signal: 75,
      uptime: 0,
      cameraReady: false,
      lastTelemetry: '--',
      memoryUsage: 52
    }
  });

  const [logs, setLogs] = useState([]);
  const logsEndRef = useRef(null);
  const [wsStatus, setWsStatus] = useState('disconnected'); // disconnected | connecting | connected
  const wsRef = useRef(null);
  const reconnectTimerRef = useRef(null);
  const [debugMode, setDebugMode] = useState(import.meta.env.VITE_DEBUG_MODE === 'true');
  const [lastRawMessage, setLastRawMessage] = useState(null);

  const mergeTelemetry = (prev, incoming) => ({
    back: { ...prev.back, ...(incoming.back || {}) },
    front: { ...prev.front, ...(incoming.front || {}) },
    camera: { ...prev.camera, ...(incoming.camera || {}) }
  });

  const connectWebSocket = () => {
    const url = import.meta.env.VITE_TELEMETRY_WS || 'ws://192.168.4.1:8888';
    try {
      setWsStatus('connecting');
      const ws = new WebSocket(url);
      wsRef.current = ws;

      ws.onopen = () => {
        setWsStatus('connected');
        setLogs(prev => [...prev, `[${new Date().toLocaleTimeString()}] WebSocket connected (${url})`]);
      };

      ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          if (debugMode) {
            setLastRawMessage({ timestamp: new Date().toISOString(), data: msg });
          }
          let update = {};
          if (msg.back || msg.front || msg.camera) {
            update = msg;
          } else if (msg.board) {
            // Support per-board messages: { board: 'back'|'front'|'camera', data: { ... } }
            update[msg.board] = msg.data || msg.payload || {};
          } else if (msg.type && msg.payload) {
            // Generic type-based messages
            if (msg.type === 'telemetry' && msg.payload.board) {
              update[msg.payload.board] = msg.payload;
            }
          }

          if (Object.keys(update).length > 0) {
            setEsp32Data(prev => mergeTelemetry(prev, update));
            setLogs(prev => [...prev.slice(-20), `[${new Date().toLocaleTimeString()}] Telemetry updated`]);
          }
        } catch (e) {
          // Non-JSON message, ignore
        }
      };

      const scheduleReconnect = () => {
        if (reconnectTimerRef.current) return; // already scheduled
        reconnectTimerRef.current = setTimeout(() => {
          reconnectTimerRef.current = null;
          connectWebSocket();
        }, 3000);
      };

      ws.onclose = () => {
        setWsStatus('disconnected');
        setLogs(prev => [...prev, `[${new Date().toLocaleTimeString()}] WebSocket disconnected`]);
        scheduleReconnect();
      };

      ws.onerror = () => {
        setWsStatus('disconnected');
        scheduleReconnect();
      };
    } catch {
      setWsStatus('disconnected');
    }
  };

  // Simulated telemetry when WebSocket is not connected
  useEffect(() => {
    if (wsStatus === 'connected') return;
    const interval = setInterval(() => {
      setEsp32Data(prev => ({
        ...prev,
        back: {
          ...prev.back,
          status: Math.random() > 0.05 ? 'connected' : 'warning',
          temperature: 40 + Math.random() * 8,
          sensors: {
            frontDist: 30 + Math.random() * 60,
            rearDist: 25 + Math.random() * 50,
            gasLevel: Math.floor(Math.random() * 300),
            gasAlertLevel: Math.random() > 0.95,
            navigationState: ['idle', 'moving', 'scanning'][Math.floor(Math.random() * 3)]
          },
          motors: {
            rearLeft: Math.floor(Math.random() * 255),
            rearRight: Math.floor(Math.random() * 255),
            isMoving: Math.random() > 0.5
          },
          cpuLoad: 40 + Math.random() * 30,
          uptime: prev.back.uptime + 1
        },
        front: {
          ...prev.front,
          status: Math.random() > 0.08 ? 'connected' : 'warning',
          temperature: 35 + Math.random() * 10,
          motors: {
            motor1: Math.floor(Math.random() * 255),
            motor2: Math.floor(Math.random() * 255),
            motor3: Math.floor(Math.random() * 255),
            motor4: Math.floor(Math.random() * 255)
          },
          cpuLoad: 20 + Math.random() * 40,
          uptime: prev.front.uptime + 1
        },
        camera: {
          ...prev.camera,
          status: Math.random() > 0.1 ? 'connected' : 'warning',
          cameraReady: Math.random() > 0.15,
          lastTelemetry: new Date().toLocaleTimeString(),
          uptime: prev.camera.uptime + 1
        }
      }));
    }, 1000);
    return () => clearInterval(interval);
  }, [wsStatus]);

  // Start WebSocket on mount
  useEffect(() => {
    connectWebSocket();
    return () => {
      if (wsRef.current) {
        try { wsRef.current.close(); } catch {}
      }
      if (reconnectTimerRef.current) {
        clearTimeout(reconnectTimerRef.current);
        reconnectTimerRef.current = null;
      }
    };
  }, []);

  // Auto-scroll logs
  useEffect(() => {
    logsEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [logs]);

  const formatUptime = (seconds) => {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return `${h}h ${m}m ${s}s`;
  };

  const StatusBadge = ({ status }) => {
    const styles = {
      connected: 'bg-green-900 text-green-200 border-green-700',
      warning: 'bg-yellow-900 text-yellow-200 border-yellow-700',
      disconnected: 'bg-red-900 text-red-200 border-red-700',
      connecting: 'bg-blue-900 text-blue-200 border-blue-700'
    };
    
    return (
      <span className={`px-2 py-1 rounded border text-xs font-mono ${styles[status]}`}>
        {status.toUpperCase()}
      </span>
    );
  };

  const MetricBox = ({ label, value, unit = '', color = 'text-blue-400', icon: Icon }) => (
    <div className="bg-gray-800/50 rounded p-3 border border-gray-700">
      <div className="flex items-center justify-between mb-2">
        <span className="text-xs text-gray-400">{label}</span>
        {Icon && <Icon size={14} className={color} />}
      </div>
      <div className={`text-lg font-bold ${color}`}>
        {typeof value === 'number' ? value.toFixed(1) : value}
        <span className="text-xs ml-1 text-gray-400">{unit}</span>
      </div>
    </div>
  );

  const MotorControl = ({ motors, boardName }) => (
    <div className="bg-gray-800/30 rounded p-3 border border-gray-700">
      <div className="text-xs font-mono text-gray-300 mb-2 uppercase">{boardName} Motors</div>
      <div className="grid grid-cols-2 gap-2">
        {Object.entries(motors).map(([key, value]) => (
          value !== undefined && typeof value === 'number' && (
            <div key={key} className="bg-gray-900/50 rounded p-2">
              <div className="text-xs text-gray-400 capitalize mb-1">{key.replace(/([A-Z])/g, ' $1')}</div>
              <div className="w-full bg-gray-700 rounded h-1.5 overflow-hidden">
                <div
                  className="h-full bg-green-500 transition-all duration-300"
                  style={{ width: `${(value / 255) * 100}%` }}
                ></div>
              </div>
              <div className="text-xs text-green-400 font-mono mt-1">{Math.floor((value / 255) * 100)}%</div>
            </div>
          )
        ))}
      </div>
    </div>
  );

  return (
    <div className="w-full h-screen bg-gray-950 text-white font-sans overflow-hidden">
      {/* Header */}
      <div className="bg-gradient-to-r from-gray-900 to-gray-800 border-b border-gray-700 px-6 py-4">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-2xl font-bold text-white">
              🚨 Project Nightfall
            </h1>
            <p className="text-xs text-gray-400 mt-1">Real-time ESP32 Network Monitor</p>
          </div>
          <div className="flex items-center gap-4">
            <div className="text-right">
              <div className="text-xs text-gray-400">Network Status</div>
              <div className="flex items-center gap-2 mt-1">
                <div className={`w-2 h-2 rounded-full ${wsStatus === 'connected' ? 'bg-green-500' : wsStatus === 'connecting' ? 'bg-yellow-400' : 'bg-red-500'} animate-pulse`}></div>
                <span className="text-sm font-mono">WS: {wsStatus.toUpperCase()}</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Main Content */}
      <div className="overflow-auto h-[calc(100vh-73px)]">
        <div className="p-6 space-y-6">
          {/* Board Status Grid */}
          <div className="grid grid-cols-1 lg:grid-cols-3 gap-4">
            {/* BACK ESP32 */}
            <div className="bg-gray-900 border border-gray-700 rounded-lg p-4 space-y-4">
              <div className="flex items-center justify-between">
                <div>
                  <h2 className="text-lg font-bold text-white">Back ESP32</h2>
                  <p className="text-xs text-gray-400">Master / Sensor Hub</p>
                </div>
                <Radio size={20} className="text-blue-400" />
              </div>

              <div>
                <StatusBadge status={esp32Data.back.status} />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <MetricBox
                  label="Battery"
                  value={esp32Data.back.battery}
                  unit="%"
                  color="text-yellow-400"
                  icon={Zap}
                />
                <MetricBox
                  label="Temp"
                  value={esp32Data.back.temperature}
                  unit="°C"
                  color="text-orange-400"
                  icon={Thermometer}
                />
                <MetricBox
                  label="Signal"
                  value={esp32Data.back.signal}
                  unit="%"
                  color="text-green-400"
                  icon={Wifi}
                />
                <MetricBox
                  label="CPU Load"
                  value={esp32Data.back.cpuLoad}
                  unit="%"
                  color="text-purple-400"
                  icon={Activity}
                />
              </div>

              {/* Sensors */}
              <div className="bg-gray-800/30 rounded p-3 border border-gray-700 space-y-2">
                <div className="text-xs font-mono text-gray-300 uppercase">Sensors</div>
                <div className="grid grid-cols-2 gap-2 text-xs">
                  <div>
                    <div className="text-gray-400">Front Distance</div>
                    <div className="text-blue-400 font-mono">{esp32Data.back.sensors.frontDist.toFixed(1)} cm</div>
                  </div>
                  <div>
                    <div className="text-gray-400">Rear Distance</div>
                    <div className="text-blue-400 font-mono">{esp32Data.back.sensors.rearDist.toFixed(1)} cm</div>
                  </div>
                  <div>
                    <div className="text-gray-400">Gas Level</div>
                    <div className={`font-mono ${esp32Data.back.sensors.gasAlertLevel ? 'text-red-400' : 'text-green-400'}`}>
                      {esp32Data.back.sensors.gasLevel}
                      {esp32Data.back.sensors.gasAlertLevel && ' ⚠️'}
                    </div>
                  </div>
                  <div>
                    <div className="text-gray-400">Nav State</div>
                    <div className="text-purple-400 font-mono capitalize">{esp32Data.back.sensors.navigationState}</div>
                  </div>
                </div>
              </div>

              {/* Rear Motors */}
              <MotorControl motors={esp32Data.back.motors} boardName="Rear" />

              <div className="text-xs text-gray-500 flex justify-between">
                <span>Uptime: {formatUptime(esp32Data.back.uptime)}</span>
                <span>Memory: {esp32Data.back.memoryUsage}%</span>
              </div>
            </div>

            {/* FRONT ESP32 */}
            <div className="bg-gray-900 border border-gray-700 rounded-lg p-4 space-y-4">
              <div className="flex items-center justify-between">
                <div>
                  <h2 className="text-lg font-bold text-white">Front ESP32</h2>
                  <p className="text-xs text-gray-400">Motor Slave</p>
                </div>
                <Gauge size={20} className="text-green-400" />
              </div>

              <div>
                <StatusBadge status={esp32Data.front.status} />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <MetricBox
                  label="Battery"
                  value={esp32Data.front.battery}
                  unit="%"
                  color="text-yellow-400"
                  icon={Zap}
                />
                <MetricBox
                  label="Temp"
                  value={esp32Data.front.temperature}
                  unit="°C"
                  color="text-orange-400"
                  icon={Thermometer}
                />
                <MetricBox
                  label="Signal"
                  value={esp32Data.front.signal}
                  unit="%"
                  color="text-green-400"
                  icon={Wifi}
                />
                <MetricBox
                  label="CPU Load"
                  value={esp32Data.front.cpuLoad}
                  unit="%"
                  color="text-purple-400"
                  icon={Activity}
                />
              </div>

              {/* Front Motors (4 motors) */}
              <MotorControl motors={esp32Data.front.motors} boardName="Front" />

              <div className="text-xs text-gray-500 flex justify-between">
                <span>Uptime: {formatUptime(esp32Data.front.uptime)}</span>
                <span>Memory: {esp32Data.front.memoryUsage}%</span>
              </div>
            </div>

            {/* CAMERA ESP32 */}
            <div className="bg-gray-900 border border-gray-700 rounded-lg p-4 space-y-4">
              <div className="flex items-center justify-between">
                <div>
                  <h2 className="text-lg font-bold text-white">Camera ESP32</h2>
                  <p className="text-xs text-gray-400">Telemetry Bridge</p>
                </div>
                <Radio size={20} className="text-cyan-400" />
              </div>

              <div>
                <StatusBadge status={esp32Data.camera.status} />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <MetricBox
                  label="Battery"
                  value={esp32Data.camera.battery}
                  unit="%"
                  color="text-yellow-400"
                  icon={Zap}
                />
                <MetricBox
                  label="Signal"
                  value={esp32Data.camera.signal}
                  unit="%"
                  color="text-green-400"
                  icon={Wifi}
                />
              </div>

              {/* Camera Status */}
              <div className="bg-gray-800/30 rounded p-3 border border-gray-700">
                <div className="flex items-center gap-2 mb-3">
                  <div className={`w-2 h-2 rounded-full ${esp32Data.camera.cameraReady ? 'bg-green-500' : 'bg-gray-500'}`}></div>
                  <span className="text-xs font-mono text-gray-300">
                    Camera: {esp32Data.camera.cameraReady ? 'READY' : 'INITIALIZING'}
                  </span>
                </div>
                <div className="text-xs text-gray-400 mb-1">Last Telemetry</div>
                <div className="text-sm text-cyan-400 font-mono">{esp32Data.camera.lastTelemetry}</div>
              </div>

              {/* ESP-NOW Messages */}
              <div className="bg-gray-800/30 rounded p-3 border border-gray-700">
                <div className="text-xs font-mono text-gray-300 mb-2 uppercase">Incoming Packets</div>
                <div className="text-sm text-green-400">
                  <div>↓ Telemetry: Active</div>
                  <div>↓ Motor Commands: {esp32Data.front.status === 'connected' ? 'Receiving' : 'Waiting'}</div>
                  <div>↓ Status: OK</div>
                </div>
              </div>

              <div className="text-xs text-gray-500 flex justify-between">
                <span>Uptime: {formatUptime(esp32Data.camera.uptime)}</span>
                <span>Memory: {esp32Data.camera.memoryUsage}%</span>
              </div>
            </div>
          </div>

          {/* System Logs */}
          <div className="bg-gray-900 border border-gray-700 rounded-lg p-4">
            <div className="flex items-center justify-between mb-3">
              <h3 className="text-lg font-bold text-white">System Events</h3>
              {debugMode && (
                <button
                  onClick={() => setDebugMode(!debugMode)}
                  className="px-2 py-1 text-xs bg-purple-900 text-purple-200 rounded border border-purple-700 hover:bg-purple-800"
                >
                  Debug ON
                </button>
              )}
            </div>
            <div className="bg-gray-950 rounded p-3 h-48 overflow-y-auto font-mono text-xs space-y-1 scrollbar-thin scrollbar-thumb-gray-700">
              {logs.length === 0 ? (
                <>
                  <div className="text-green-500">[12:45:32] Back ESP32 connected ✓</div>
                  <div className="text-green-500">[12:45:33] Front ESP32 connected ✓</div>
                  <div className="text-green-500">[12:45:34] Camera ESP32 connected ✓</div>
                  <div className="text-blue-400">[12:45:35] ESP-NOW telemetry active</div>
                  <div className="text-blue-400">[12:46:10] Telemetry: FrontDist=45.2cm RearDist=38.1cm</div>
                  <div className="text-blue-400">[12:46:11] Motor command: forward speed=180</div>
                  <div className="text-blue-400">[12:46:12] All motors executing</div>
                  <div className="text-yellow-400">[12:46:35] Gas sensor reading spike (trending up)</div>
                  <div className="text-blue-400">[12:46:40] Autonomous navigation: SCANNING</div>
                </>
              ) : (
                logs.map((log, i) => (
                  <div key={i} className="text-blue-400">{log}</div>
                ))
              )}
              <div ref={logsEndRef} />
            </div>
          </div>

          {/* Debug Panel */}
          {debugMode && lastRawMessage && (
            <div className="bg-gray-900 border border-purple-700 rounded-lg p-4">
              <h3 className="text-lg font-bold text-purple-400 mb-3">Debug: Last Raw Message</h3>
              <div className="bg-gray-950 rounded p-3 overflow-auto max-h-64">
                <pre className="text-xs text-purple-200 font-mono">
                  {JSON.stringify(lastRawMessage, null, 2)}
                </pre>
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};

export default ESP32Dashboard;
