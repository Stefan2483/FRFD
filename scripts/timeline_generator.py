#!/usr/bin/env python3
"""
FRFD Timeline Generator
Generates forensic timelines from collected artifacts
"""

import json
import csv
import sys
import os
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Any
import argparse

class TimelineEvent:
    def __init__(self, timestamp: str, source: str, event_type: str, description: str, details: Dict = None):
        self.timestamp = timestamp
        self.source = source
        self.event_type = event_type
        self.description = description
        self.details = details or {}

    def to_dict(self):
        return {
            'timestamp': self.timestamp,
            'source': source,
            'event_type': self.event_type,
            'description': self.description,
            'details': self.details
        }

class TimelineGenerator:
    def __init__(self, evidence_path: str):
        self.evidence_path = Path(evidence_path)
        self.events: List[TimelineEvent] = []

    def parse_windows_events(self):
        """Parse Windows event logs"""
        print("[Timeline] Parsing Windows event logs...")

        # Look for CSV files from event log collection
        event_files = list(self.evidence_path.glob("**/EventLogs_*/Security.csv"))
        event_files += list(self.evidence_path.glob("**/EventLogs_*/System.csv"))
        event_files += list(self.evidence_path.glob("**/EventLogs_*/Application.csv"))

        for file_path in event_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    reader = csv.DictReader(f)
                    for row in reader:
                        try:
                            event = TimelineEvent(
                                timestamp=row.get('TimeCreated', ''),
                                source='Windows Event Log',
                                event_type=row.get('LevelDisplayName', 'Unknown'),
                                description=f"Event ID {row.get('Id', 'N/A')}: {row.get('Message', '')[:100]}",
                                details={
                                    'log_name': file_path.stem,
                                    'event_id': row.get('Id', ''),
                                    'provider': row.get('ProviderName', '')
                                }
                            )
                            self.events.append(event)
                        except Exception as e:
                            continue
            except Exception as e:
                print(f"[Timeline] Error parsing {file_path}: {e}")

    def parse_prefetch(self):
        """Parse Windows Prefetch analysis"""
        print("[Timeline] Parsing Prefetch data...")

        prefetch_files = list(self.evidence_path.glob("**/Prefetch_*/prefetch_analysis.csv"))

        for file_path in prefetch_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    reader = csv.DictReader(f)
                    for row in reader:
                        event = TimelineEvent(
                            timestamp=row.get('Modified', ''),
                            source='Prefetch',
                            event_type='Program Execution',
                            description=f"Executed: {row.get('ExecutableName', 'Unknown')}",
                            details={
                                'file_name': row.get('FileName', ''),
                                'created': row.get('Created', ''),
                                'size': row.get('Size', '')
                            }
                        )
                        self.events.append(event)
            except Exception as e:
                print(f"[Timeline] Error parsing {file_path}: {e}")

    def parse_network_connections(self):
        """Parse network connection logs"""
        print("[Timeline] Parsing network connections...")

        network_files = list(self.evidence_path.glob("**/NetworkConnections_*/tcp_connections.csv"))

        for file_path in network_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    reader = csv.DictReader(f)
                    for row in reader:
                        if row.get('CreationTime'):
                            event = TimelineEvent(
                                timestamp=row.get('CreationTime', ''),
                                source='Network Connection',
                                event_type='Connection Established',
                                description=f"{row.get('ProcessName', 'Unknown')} -> {row.get('RemoteAddress', '')}:{row.get('RemotePort', '')}",
                                details={
                                    'local_address': row.get('LocalAddress', ''),
                                    'local_port': row.get('LocalPort', ''),
                                    'state': row.get('State', ''),
                                    'pid': row.get('OwningProcess', '')
                                }
                            )
                            self.events.append(event)
            except Exception as e:
                print(f"[Timeline] Error parsing {file_path}: {e}")

    def parse_linux_auth_logs(self):
        """Parse Linux authentication logs"""
        print("[Timeline] Parsing Linux auth logs...")

        # Parse last logins
        last_files = list(self.evidence_path.glob("**/AuthLogs_*/last_logins.txt"))

        for file_path in last_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    for line in f:
                        if line.strip() and not line.startswith('wtmp'):
                            parts = line.split()
                            if len(parts) >= 5:
                                event = TimelineEvent(
                                    timestamp=' '.join(parts[3:8]) if len(parts) >= 8 else parts[3],
                                    source='Linux Auth Log',
                                    event_type='User Login',
                                    description=f"User {parts[0]} logged in from {parts[2]}",
                                    details={
                                        'username': parts[0],
                                        'tty': parts[1],
                                        'source': parts[2]
                                    }
                                )
                                self.events.append(event)
            except Exception as e:
                print(f"[Timeline] Error parsing {file_path}: {e}")

    def parse_scheduled_tasks(self):
        """Parse scheduled tasks"""
        print("[Timeline] Parsing scheduled tasks...")

        task_files = list(self.evidence_path.glob("**/ScheduledTasks_*/scheduled_tasks.csv"))

        for file_path in task_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    reader = csv.DictReader(f)
                    for row in reader:
                        if row.get('LastRunTime') and row.get('LastRunTime') != '-':
                            event = TimelineEvent(
                                timestamp=row.get('LastRunTime', ''),
                                source='Scheduled Task',
                                event_type='Task Execution',
                                description=f"Task: {row.get('TaskName', 'Unknown')}",
                                details={
                                    'state': row.get('State', ''),
                                    'user': row.get('UserId', ''),
                                    'actions': row.get('Actions', ''),
                                    'suspicious': row.get('IsSuspicious', 'False')
                                }
                            )
                            self.events.append(event)
            except Exception as e:
                print(f"[Timeline] Error parsing {file_path}: {e}")

    def parse_all_sources(self):
        """Parse all available forensic sources"""
        print(f"[Timeline] Parsing evidence from: {self.evidence_path}")

        self.parse_windows_events()
        self.parse_prefetch()
        self.parse_network_connections()
        self.parse_linux_auth_logs()
        self.parse_scheduled_tasks()

        print(f"[Timeline] Total events collected: {len(self.events)}")

    def sort_events(self):
        """Sort events chronologically"""
        print("[Timeline] Sorting events chronologically...")

        def parse_timestamp(event):
            try:
                # Try multiple date formats
                formats = [
                    '%Y-%m-%dT%H:%M:%S',
                    '%Y-%m-%d %H:%M:%S',
                    '%m/%d/%Y %H:%M:%S',
                    '%Y-%m-%d %H:%M',
                ]

                for fmt in formats:
                    try:
                        return datetime.strptime(event.timestamp[:19], fmt)
                    except:
                        continue

                return datetime.min
            except:
                return datetime.min

        self.events.sort(key=parse_timestamp)

    def export_csv(self, output_path: str):
        """Export timeline to CSV"""
        print(f"[Timeline] Exporting to CSV: {output_path}")

        with open(output_path, 'w', newline='', encoding='utf-8') as f:
            fieldnames = ['timestamp', 'source', 'event_type', 'description']
            writer = csv.DictWriter(f, fieldnames=fieldnames)

            writer.writeheader()
            for event in self.events:
                writer.writerow({
                    'timestamp': event.timestamp,
                    'source': event.source,
                    'event_type': event.event_type,
                    'description': event.description
                })

    def export_json(self, output_path: str):
        """Export timeline to JSON"""
        print(f"[Timeline] Exporting to JSON: {output_path}")

        timeline_data = {
            'metadata': {
                'generated': datetime.now().isoformat(),
                'evidence_path': str(self.evidence_path),
                'total_events': len(self.events)
            },
            'events': [
                {
                    'timestamp': event.timestamp,
                    'source': event.source,
                    'event_type': event.event_type,
                    'description': event.description,
                    'details': event.details
                }
                for event in self.events
            ]
        }

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(timeline_data, f, indent=2, ensure_ascii=False)

    def export_html(self, output_path: str):
        """Export timeline to interactive HTML"""
        print(f"[Timeline] Exporting to HTML: {output_path}")

        html = """
<!DOCTYPE html>
<html>
<head>
    <title>FRFD Forensic Timeline</title>
    <meta charset="UTF-8">
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            background: #1a1a1a;
            color: #e0e0e0;
        }
        .container {
            max-width: 1400px;
            margin: 0 auto;
        }
        h1 {
            color: #3498db;
            text-align: center;
            margin-bottom: 10px;
        }
        .stats {
            background: #2c2c2c;
            padding: 15px;
            border-radius: 5px;
            margin-bottom: 20px;
            display: flex;
            justify-content: space-around;
        }
        .stat-item {
            text-align: center;
        }
        .stat-value {
            font-size: 24px;
            font-weight: bold;
            color: #3498db;
        }
        .timeline {
            position: relative;
            padding: 20px 0;
        }
        .timeline-event {
            background: #2c2c2c;
            padding: 15px;
            margin: 10px 0;
            border-left: 4px solid #3498db;
            border-radius: 5px;
            transition: transform 0.2s;
        }
        .timeline-event:hover {
            transform: translateX(5px);
            box-shadow: 0 4px 8px rgba(52, 152, 219, 0.3);
        }
        .event-header {
            display: flex;
            justify-content: space-between;
            margin-bottom: 10px;
        }
        .event-timestamp {
            color: #3498db;
            font-weight: bold;
        }
        .event-source {
            color: #95a5a6;
            font-size: 12px;
        }
        .event-type {
            background: #34495e;
            padding: 3px 8px;
            border-radius: 3px;
            font-size: 12px;
        }
        .event-description {
            margin-top: 10px;
            line-height: 1.4;
        }
        .filter-section {
            background: #2c2c2c;
            padding: 15px;
            border-radius: 5px;
            margin-bottom: 20px;
        }
        .filter-section input, .filter-section select {
            background: #1a1a1a;
            border: 1px solid #34495e;
            color: #e0e0e0;
            padding: 8px;
            border-radius: 3px;
            margin-right: 10px;
        }
        button {
            background: #3498db;
            color: white;
            border: none;
            padding: 8px 15px;
            border-radius: 3px;
            cursor: pointer;
        }
        button:hover {
            background: #2980b9;
        }
    </style>
    <script>
        function filterTimeline() {
            const searchTerm = document.getElementById('search').value.toLowerCase();
            const sourceFilter = document.getElementById('sourceFilter').value;

            const events = document.getElementsByClassName('timeline-event');

            for (let event of events) {
                const text = event.textContent.toLowerCase();
                const source = event.getAttribute('data-source');

                const matchesSearch = searchTerm === '' || text.includes(searchTerm);
                const matchesSource = sourceFilter === 'all' || source === sourceFilter;

                event.style.display = (matchesSearch && matchesSource) ? 'block' : 'none';
            }
        }

        function exportData(format) {
            alert('Export to ' + format + ' - Check console logs for implementation');
        }
    </script>
</head>
<body>
    <div class="container">
        <h1>📊 FRFD Forensic Timeline</h1>

        <div class="stats">
            <div class="stat-item">
                <div class="stat-value">""" + str(len(self.events)) + """</div>
                <div>Total Events</div>
            </div>
            <div class="stat-item">
                <div class="stat-value">""" + str(len(set(e.source for e in self.events))) + """</div>
                <div>Data Sources</div>
            </div>
            <div class="stat-item">
                <div class="stat-value">""" + datetime.now().strftime("%Y-%m-%d") + """</div>
                <div>Generated</div>
            </div>
        </div>

        <div class="filter-section">
            <input type="text" id="search" placeholder="Search timeline..." onkeyup="filterTimeline()" style="width: 300px;">
            <select id="sourceFilter" onchange="filterTimeline()">
                <option value="all">All Sources</option>"""

        sources = sorted(set(e.source for e in self.events))
        for source in sources:
            html += f'<option value="{source}">{source}</option>'

        html += """
            </select>
            <button onclick="exportData('CSV')">Export CSV</button>
            <button onclick="exportData('JSON')">Export JSON</button>
        </div>

        <div class="timeline">"""

        for event in self.events:
            html += f"""
            <div class="timeline-event" data-source="{event.source}">
                <div class="event-header">
                    <span class="event-timestamp">{event.timestamp}</span>
                    <span class="event-type">{event.event_type}</span>
                </div>
                <div class="event-source">{event.source}</div>
                <div class="event-description">{event.description}</div>
            </div>"""

        html += """
        </div>
    </div>
</body>
</html>"""

        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(html)

def main():
    parser = argparse.ArgumentParser(description='FRFD Timeline Generator')
    parser.add_argument('evidence_path', help='Path to evidence directory')
    parser.add_argument('-o', '--output', default='timeline', help='Output file prefix')
    parser.add_argument('-f', '--format', choices=['csv', 'json', 'html', 'all'], default='all', help='Output format')

    args = parser.parse_args()

    print("=== FRFD Timeline Generator ===")
    print(f"Evidence Path: {args.evidence_path}")

    generator = TimelineGenerator(args.evidence_path)
    generator.parse_all_sources()
    generator.sort_events()

    if args.format in ['csv', 'all']:
        generator.export_csv(f"{args.output}.csv")

    if args.format in ['json', 'all']:
        generator.export_json(f"{args.output}.json")

    if args.format in ['html', 'all']:
        generator.export_html(f"{args.output}.html")

    print("\n[Timeline] Generation complete!")
    print(f"[Timeline] Events processed: {len(generator.events)}")
    print(f"[Timeline] Output files: {args.output}.*")

if __name__ == '__main__':
    main()
