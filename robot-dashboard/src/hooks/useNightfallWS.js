import { useState, useEffect, useRef, useCallback } from 'react';

const PACKET_TYPES = {
  TELEMETRY: 'telemetry',
  STATUS: 'status',
  PING: 'ping'
};

const WS_URL = 'ws://192.168.4.1:8888';
const RECONNECT_DELAY = 2500;

export const useNightfallWS = () => {
  const [telemetry, setTelemetry] = useState({
    sensors: {
      front_dist: 0,
      rear_dist: 0,
      gas: 0
    },
    motors: {
      front_left: 0,
      front_right: 0,
      rear_left: 0,
      rear_right: 0
    },
    state: {
      fsm: 'INIT',
      autonomous: false,
      nav_state: 'idle'
    },
    network: {
      front: false,
      camera: false
    },
    control: {
      out: 0,
      err: 0,
      sp: 0,
      P: 0,
      I: 0,
      D: 0
    },
    timing: {
      loop_us: 0
    },
    server_clients: 0,
    lastUpdate: 0
  });

  const [connectionStatus, setConnectionStatus] = useState('disconnected'); // disconnected, connected, error
  const [lastPing, setLastPing] = useState(0);
  const [connectionStats, setConnectionStats] = useState({
    msgRate: 0,
    msgsReceived: 0,
    lastMsgTime: 0,
    connectedSince: null,
    reconnectAttempts: 0,
    lastError: null
  });

  const wsRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);
  const msgCountRef = useRef(0);
  const lastRateCheckRef = useRef(Date.now());

  // Calculate Message Rate
  useEffect(() => {
    const interval = setInterval(() => {
      const now = Date.now();
      const elapsed = (now - lastRateCheckRef.current) / 1000;
      if (elapsed > 0) {
        const rate = msgCountRef.current / elapsed;
        setConnectionStats(prev => ({ ...prev, msgRate: Math.round(rate) }));
        msgCountRef.current = 0;
        lastRateCheckRef.current = now;
      }
    }, 1000);
    return () => clearInterval(interval);
  }, []);

  const connect = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN) return;

    console.log(`[WS] Connecting to ${WS_URL}...`);
    const ws = new WebSocket(WS_URL);
    wsRef.current = ws;

    ws.onopen = () => {
      console.log('[WS] Connected');
      setConnectionStatus('connected');
      setConnectionStats(prev => ({ 
        ...prev, 
        connectedSince: Date.now(), 
        reconnectAttempts: 0, 
        lastError: null 
      }));
    };

    ws.onmessage = (event) => {
      try {
        msgCountRef.current++;
        const data = JSON.parse(event.data);
        const now = Date.now();
        
        setConnectionStats(prev => ({ 
          ...prev, 
          msgsReceived: prev.msgsReceived + 1, 
          lastMsgTime: now 
        }));

        if (data.type === PACKET_TYPES.TELEMETRY) {
          setTelemetry(prev => ({
            ...prev,
            sensors: data.sensors || prev.sensors,
            motors: data.motors || prev.motors,
            state: data.state || prev.state,
            network: data.network || prev.network,
            control: data.control || prev.control,
            timing: data.timing || prev.timing,
            server_clients: data.server_clients || 0,
            lastUpdate: now
          }));
          
          if (data.ts) {
             setLastPing(now - data.ts); // Approximate latency if clocks synced roughly (or just purely interval)
          }
        }
      } catch (err) {
        console.error('[WS] Parse error:', err);
      }
    };

    ws.onclose = (e) => {
      console.log('[WS] Disconnected', e.reason);
      setConnectionStatus('disconnected');
      setConnectionStats(prev => ({ ...prev, connectedSince: null }));
      attemptReconnect();
    };

    ws.onerror = (err) => {
      console.error('[WS] Error:', err);
      setConnectionStatus('error');
      setConnectionStats(prev => ({ ...prev, lastError: 'Connection Failed' }));
      ws.close();
    };

  }, []);

  const attemptReconnect = () => {
    if (reconnectTimeoutRef.current) clearTimeout(reconnectTimeoutRef.current);
    
    setConnectionStats(prev => ({ ...prev, reconnectAttempts: prev.reconnectAttempts + 1 }));

    reconnectTimeoutRef.current = setTimeout(() => {
      console.log('[WS] Attempting Reconnect...');
      connect();
    }, RECONNECT_DELAY);
  };

  const sendCommand = useCallback((cmdType, payload = {}) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      const msg = {
        type: cmdType,
        ...payload
      };
      wsRef.current.send(JSON.stringify(msg));
    } else {
      console.warn('[WS] Cannot send, not connected');
    }
  }, []);

  const sendUiCmd = useCallback((cmdString, payload = {}) => {
    sendCommand('ui_cmd', { cmd: cmdString, ...payload });
  }, [sendCommand]);

  useEffect(() => {
    connect();
    return () => {
      if (wsRef.current) wsRef.current.close();
      if (reconnectTimeoutRef.current) clearTimeout(reconnectTimeoutRef.current);
    };
  }, [connect]);

  return {
    telemetry,
    connectionStatus,
    connectionStats,
    sendUiCmd,
    lastPing
  };
};
