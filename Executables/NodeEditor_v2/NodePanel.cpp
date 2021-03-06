#include "NodePanel.hpp"


namespace inl::tool {



NodePanel::NodePanel() {
	m_layout.SetReferencePoint(gui::AbsoluteLayout::eRefPoint::CENTER);
	m_layout.SetYDown(false);
	SetLayout(m_layout);

	OnDragBegin += Delegate<void(Control*, Vec2)>{ &NodePanel::OnNodeDragBegin, this };
	OnDrag += Delegate<void(Control*, Vec2)>{ &NodePanel::OnNodeDragged, this };
	OnDragEnd += Delegate<void(Control*, Vec2, Control*)>{ &NodePanel::OnNodeDragEnd, this };

	OnDragBegin += Delegate<void(Control*, Vec2)>{ &NodePanel::OnPortDragBegin, this };
	OnDrag += Delegate<void(Control*, Vec2)>{ &NodePanel::OnPortDragged, this };
	OnDragEnd += Delegate<void(Control*, Vec2, Control*)>{ &NodePanel::OnPortDragEnd, this };

	OnDragBegin += Delegate<void(Control*, Vec2)>{ &NodePanel::OnPanViewBegin, this };
	OnDrag += Delegate<void(Control*, Vec2)>{ &NodePanel::OnPanView, this };
}


void NodePanel::AddNode(std::shared_ptr<NodeControl> node) {
	auto [it, isNew] = m_nodes.insert(std::move(node));
	m_layout.AddChild(*it).SetPosition({ 0, 0 });
}


void NodePanel::RemoveNode(const NodeControl* node) {
	auto it = m_nodes.find(node);
	[[likely]] if (it != m_nodes.end()) {
		m_nodes.erase(it);
	}
	else {
		throw InvalidArgumentException("Node is not shown on this panel.");
	}
}


void NodePanel::AddLink(const NodeControl* source, int sourcePort, const NodeControl* target, int targetPort) {
	ArrowKey key{ source, sourcePort, target, targetPort };
	auto [it, isNew] = m_arrows.insert({ key, std::make_shared<ArrowControl>() });
	if (isNew) {
		m_layout.AddChild(it->second).MoveToFront();
		UpdateArrowPosition(it->first, *it->second);
	}
}


void NodePanel::RemoveLink(const NodeControl* source, int sourcePort, const NodeControl* target, int targetPort) {
	ArrowKey key{ source, sourcePort, target, targetPort };
	auto it = m_arrows.find(key);
	[[likely]] if (it != m_arrows.end()) {
		m_arrows.erase(it);
	}
	else {
		throw InvalidArgumentException("There is no link between specified nodes.");
	}
}

void NodePanel::Clear() {
	m_layout.Clear();
	m_arrows.clear();
	m_nodes.clear();
	m_draggedNode = nullptr;
	m_draggedPort = nullptr;
}


void NodePanel::OffsetAllNodes(Vec2 offset) {
	for (auto& node : m_nodes) {
		auto& binding = m_layout[node.get()];
		binding.SetPosition(binding.GetPosition() + offset);
	}
}


void NodePanel::UpdateArrowPositions() {
	for (auto& arrow : m_arrows) {
		UpdateArrowPosition(arrow.first, *arrow.second.get());
	}
}


void NodePanel::UpdateArrowPosition(const ArrowKey& key, ArrowControl& arrow) {
	auto& sourcePort = key.source->GetOutputPort(key.sourcePort);
	auto& targetPort = key.target->GetInputPort(key.targetPort);

	Vec2 begin = sourcePort.GetPosition() + Vec2{ sourcePort.GetSize().x / 2.0f, 0 };
	Vec2 end = targetPort.GetPosition() - Vec2{ targetPort.GetSize().x / 2.0f, 0 };

	arrow.SetEndPoints(begin, end);
}


void NodePanel::OnNodeDragBegin(Control* control, Vec2 dragOrigin) {
	if (NodeControl * node; node = dynamic_cast<NodeControl*>(control)) {
		m_dragOffset = node->GetPosition() - dragOrigin;
		m_draggedNode = node;
	}
}


void NodePanel::OnNodeDragged(Control* control, Vec2 dragPosition) {
	if (NodeControl * node; node = dynamic_cast<NodeControl*>(control)) {
		m_layout[node].SetPosition(m_dragOffset + dragPosition - m_layout.GetPosition());
		UpdateArrowPositions();
	}
}


void NodePanel::OnNodeDragEnd(Control* control, Vec2 dragEnd, Control*) {
	if (NodeControl * node; node = dynamic_cast<NodeControl*>(control)) {
		m_draggedNode = nullptr;
	}
}


void NodePanel::OnPortDragBegin(Control* control, Vec2 dragOrigin) {
	if (PortControl * port; port = dynamic_cast<PortControl*>(control)) {
		if (m_draggedPort) {
			m_layout.RemoveChild(m_draggedPort);
		}
		m_layout.AddChild(m_temporaryArrow).MoveToFront();
		m_draggedPort = port;
	}
}


void NodePanel::OnPortDragged(Control* control, Vec2 dragTarget) {
	if (PortControl * port; port = dynamic_cast<PortControl*>(control)) {
		float side = port->IsOutput() ? +1.f : -1.f;
		Vec2 begin = port->GetPosition();
		begin.x += 0.5f * side * port->GetSize().x;
		Vec2 end = dragTarget;
		if (port->IsInput()) {
			std::swap(begin, end);
		}
		m_temporaryArrow.SetEndPoints(begin, end);
	}
}


void NodePanel::OnPortDragEnd(Control* control, Vec2 dragEnd, Control* target) {
	if (PortControl * port; port = dynamic_cast<PortControl*>(control)) {
		m_layout.RemoveChild(&m_temporaryArrow);
		PortControl* targetPort = dynamic_cast<PortControl*>(target);
		if (targetPort && (port->IsInput() ^ targetPort->IsInput())) {
			if (targetPort->IsOutput()) {
				std::swap(targetPort, port);
			}
			OnAddLink(port->GetNode(), port->GetPortIndex(), targetPort->GetNode(), targetPort->GetPortIndex());
		}
		m_draggedPort = nullptr;
	}
}


void NodePanel::OnPanViewBegin(Control* control, Vec2 dragOrigin) {
	if (control == &m_layout) {
		m_panOrigin = dragOrigin;
	}
}


void NodePanel::OnPanView(Control* control, Vec2 dragTarget) {
	if (control == &m_layout) {
		Vec2 offset = dragTarget - m_panOrigin;
		m_panOrigin = dragTarget;
		OffsetAllNodes(offset);
		UpdateArrowPositions();
	}
}


} // namespace inl::tool