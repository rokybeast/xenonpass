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
	vault_path: string;
	master_password: string;
	name: string;
	username: string;
	password: string;
	url: string;
	notes: string;
}

export interface VaultRequest {
	vault_path: string;
	master_password: string;
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

export async function listEntries(req: VaultRequest): Promise<EntryListResponse> {
	const res = await fetch(`${API_BASE}/entries`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(req)
	});
	if (!res.ok) throw new Error(`list entries failed: ${res.status}`);
	return res.json();
}

export async function createEntry(req: EntryRequest): Promise<Entry> {
	const res = await fetch(`${API_BASE}/entries/create`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(req)
	});
	if (!res.ok) throw new Error(`create entry failed: ${res.status}`);
	return res.json();
}

export async function deleteEntry(req: VaultRequest & { id: string }): Promise<void> {
	const res = await fetch(`${API_BASE}/entries/delete`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(req)
	});
	if (!res.ok) throw new Error(`delete entry failed: ${res.status}`);
}
