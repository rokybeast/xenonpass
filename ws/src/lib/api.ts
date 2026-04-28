const API_BASE = '/api';

export interface Entry {
	id: string;
	name: string;
	username: string;
	password: string;
	url: string;
	notes: string;
}

export interface EntryRequest {
	name: string;
	username: string;
	password: string;
	url: string;
	notes: string;
}

export interface HealthResponse {
	status: string;
	version: string;
}

export interface EntryListResponse {
	entries: Entry[];
	count: number;
}

export async function getHealth(): Promise<HealthResponse> {
	const res = await fetch(`${API_BASE}/health`);
	if (!res.ok) throw new Error(`health check failed: ${res.status}`);
	return res.json();
}

export async function listEntries(): Promise<EntryListResponse> {
	const res = await fetch(`${API_BASE}/entries`);
	if (!res.ok) throw new Error(`list entries failed: ${res.status}`);
	return res.json();
}

export async function createEntry(entry: EntryRequest): Promise<Entry> {
	const res = await fetch(`${API_BASE}/entries/create`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(entry)
	});
	if (!res.ok) throw new Error(`create entry failed: ${res.status}`);
	return res.json();
}

export async function getEntry(id: string): Promise<Entry> {
	const res = await fetch(`${API_BASE}/entries/get?id=${encodeURIComponent(id)}`);
	if (!res.ok) throw new Error(`get entry failed: ${res.status}`);
	return res.json();
}

export async function deleteEntry(id: string): Promise<void> {
	const res = await fetch(`${API_BASE}/entries/delete?id=${encodeURIComponent(id)}`, {
		method: 'DELETE'
	});
	if (!res.ok) throw new Error(`delete entry failed: ${res.status}`);
}
